/*
 * Copyright 2009, Yahoo!
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 *  3. Neither the name of Yahoo! nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ImageProcessor.hh"
#include "util/fileutil.hh"
#include "magick/api.h"

#include "service.hh"

#include <sstream>

// a map of supported types.
// written in init, and only read after that (for threading issues)
struct CaseInsensitiveCompare 
{
    bool operator()(const std::string& lhs, const std::string& rhs) const 
    {
        return(strcasecmp(lhs.c_str(), rhs.c_str()) < 0);
    }
};

typedef std::map<std::string, std::string, CaseInsensitiveCompare> ExtMap;
static ExtMap s_imgFormats;

const imageproc::Type imageproc::UNKNOWN = NULL;

void
imageproc::init()
{
    RegisterStaticModules();
    InitializeMagick(NULL);

    // let's output a startup banner with available image type support
    ExceptionInfo exception;
    MagickInfo ** arr = GetMagickInfoArray( &exception );
    std::stringstream ss;
    
    ss << "GraphicsMagick engine initialized with support for: [ ";

    bool first = true;
    while (arr && *arr) {
        if (!first) ss << ", ";
        first = false;
        ss << (*arr)->name;
        char * mt = MagickToMime( (*arr)->name );
        if (mt) {
            s_imgFormats[(*arr)->name] = std::string(mt);
            ss << " (" << mt << ")";
            free(mt);
        }
        arr++;
    }
    ss << " ]";

    g_bpCoreFunctions->log(BP_INFO, ss.str().c_str());
}

void
imageproc::shutdown()
{
    DestroyMagick();
}

#ifdef WIN32
#define strcasecmp _stricmp
#endif

imageproc::Type
imageproc::pathToType(const std::string & path)
{
    Type rval = UNKNOWN;

    if (!path.empty())
    {
        size_t pos = path.rfind('.');
        if (pos == std::string::npos) pos = -1;
        std::string ext = path.substr(pos+1, std::string::npos);
        
        ExtMap::const_iterator it = s_imgFormats.find(ext);
        if (it != s_imgFormats.end()) {
            rval = it->first.c_str();
        }
    }

    return rval;
}

std::string
imageproc::typeToExt(Type t)
{
    std::string ext;
    while (t && *t) { ext.append(1, (char) tolower(*t++)); }
    return ext;
}


std::string
imageproc::ChangeImage(const std::string & inPath,
                       const std::string & tmpDir,
                       Type outputFormat,
                       const bp::List & transformations,
                       int quality,
                       std::string & oError)
{
    
    
    ExceptionInfo exception;
    Image *images;
    ImageInfo *image_info;
    
    GetExceptionInfo(&exception);
    
    // first we read the image
    g_bpCoreFunctions->log(
        BP_INFO, "Attempting to read image: %s",
        inPath.c_str());

    image_info = CloneImageInfo((ImageInfo *) NULL);
    (void) strcpy(image_info->filename, inPath.c_str());
    images = ReadImage(image_info, &exception);

    // XXX: win32 non-ascii paths?
    
    g_bpCoreFunctions->log(
        BP_INFO, "Contains %lu frames, type: %s\n",
        GetImageListLength(images),
        images->magick);

    if (exception.severity != UndefinedException)
    {
        CatchException(&exception);
    }
    
    if (!images)
    {
        oError.append("couldn't read image");
        DestroyImageInfo(image_info);
        image_info = NULL;
        DestroyExceptionInfo(&exception);
        return std::string();
    }
    
    // set quality
    if (quality > 100) quality = 100;
    if (quality < 0) quality = 0;
    image_info->quality = quality;

    g_bpCoreFunctions->log(
        BP_INFO, "Transformation performed at %d quality (0-100)",
        quality);

    // XXX: this is where we'll run all of the processing

    // let's set the output format correctly (default to input format)
    std::string name;
    if (outputFormat == UNKNOWN) name.append(ft::basename(inPath));
    else {
        name.append("img.");
        name.append(typeToExt(outputFormat));
        (void) sprintf(images->magick, outputFormat);
        g_bpCoreFunctions->log(BP_INFO, "Output to format: %s", outputFormat);
    }
    
    // XXX: is there such a function for GM?
    //
    // "StripImage() strips an image of all profiles and comments."
    // (for size)
    // StripImage(images);

    // Now let's go directly from blob to file.  We bypass
    // GM to-file functions so that we can handle wide filenames
    // safely on win32 systems.  A superior solution would
    // be to use GM stream facilities (if they exist)
    
    // upon success, will hold path to output file and will be returned to
    // client
    std::string rv;
    
    {
        size_t l = 0;
        void * blob = NULL;
        blob = ImageToBlob( image_info, images, &l, &exception );

        if (exception.severity != UndefinedException)
        {
            oError.append("ImageToBlob failed.");
            CatchException(&exception);
        }
        else
        {
            g_bpCoreFunctions->log(BP_INFO, "Writing %lu bytes to %s",
                                   l, name.c_str());

            if (!ft::mkdir(tmpDir, false)) {
                oError.append("Couldn't create temp dir");
            } else {
                std::string outpath = ft::getPath(tmpDir, name);
                FILE * f = ft::fopen_binary_write(outpath);
                if (f == NULL) { 
                    g_bpCoreFunctions->log(
                        BP_ERROR, "Couldn't open '%s' for writing!",
                        outpath.c_str());
                    oError.append("Error saving output image");
                } else {
                    size_t wt;
                    wt = fwrite(blob, sizeof(char), l, f);
                    fclose(f);

                    if (wt != l) {
                        g_bpCoreFunctions->log(
                            BP_ERROR,
                            "Partial write (%lu/%lu) when writing resultant "
                            "image '%s'",
                            wt, l, outpath.c_str());
                        oError.append("Error saving output image");
                    } else {
                        // success!
                        rv = outpath;
                    }
                }
            }
        }
    }
    
    DestroyImage(images);
    DestroyImageInfo(image_info);
    image_info = NULL;
    DestroyExceptionInfo(&exception);

    return rv;
}
