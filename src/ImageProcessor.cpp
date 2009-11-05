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

void
imageproc::init()
{
    RegisterStaticModules();
    InitializeMagick(NULL);
}

void
imageproc::shutdown()
{
    DestroyMagick();
}

imageproc::Type
imageproc::pathToType(const std::string & path)
{
    static struct
    {
        const char * ext;
        Type type;
    }
    s_exts[] = {
        { "jpg", JPEG },
        { "jpeg", JPEG },
        { "gif", GIF },
        { "gif", PNG }
    };
    
    for (unsigned int i = 0; i < sizeof(s_exts)/sizeof(s_exts[0]); i++)
    {
        if (path.length() >= strlen(s_exts[i].ext)) {
            std::string ss =
                path.substr(path.length() - strlen(s_exts[i].ext));
            if (!ss.compare(std::string(s_exts[i].ext))) {
                return s_exts[i].type;
            }
        }
    }
    
    return UNKNOWN;
}

std::string
imageproc::typeToExt(Type t)
{
    std::string rv;
    switch (t) {
        case JPEG: rv.append("jpg"); break;
        case PNG: rv.append("png"); break;
        case GIF: rv.append("gif"); break;
        case UNKNOWN: break; // noop
    }
    return rv;
}


std::string
imageproc::ChangeImage(const std::string & inPath,
                       const std::string & tmpDir,
                       Type outputFormat,
                       const bp::List & transformations,
                       std::string & oError)
{
    ExceptionInfo exception;
    Image *images;
    ImageInfo *image_info;
    
    assert(IsMagickInstantiated());
    
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
        BP_INFO, "Contains %lu frames\n",
        GetImageListLength(images));

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

    // XXX: this is where we'll run all of the processing


    // let's set the output format correctly (default to input format)
    std::string name;
    if (outputFormat == UNKNOWN) name.append(ft::basename(inPath));
    else {
        name.append("img.");
        name.append(typeToExt(outputFormat));
    }
    (void) sprintf(images->filename, name.c_str());
    

    // "StripImage() strips an image of all profiles and comments."
    // (for size)
    // StripImage(images);

    // Write the image data into string 'output'.
    std::string output;

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
            output.append((const char *) blob, l);
        }
    }
    
    DestroyImage(images);
    DestroyImageInfo(image_info);
    image_info = NULL;
    DestroyExceptionInfo(&exception);

    g_bpCoreFunctions->log(BP_INFO, "Writing %lu bytes to %s",
                           output.length(), name.c_str());

    if (!ft::mkdir(tmpDir, false)) {
        oError.append("Couldn't create temp dir");
        return std::string();
    }

    std::string outpath = ft::getPath(tmpDir, name);
    FILE * f = ft::fopen_binary_write(outpath);
    if (f == NULL) { 
        g_bpCoreFunctions->log(BP_ERROR, "Couldn't open '%s' for writing!",
                               outpath.c_str());
        oError.append("Error saving output image");
        return std::string();
    }

    size_t wt;
    wt = fwrite((void *) output.c_str(), sizeof(char), output.length(), f);
    fclose(f);
    
    if (wt != output.length()) {
        g_bpCoreFunctions->log(
            BP_ERROR,
            "Partial write (%lu/%lu) when writing resultant image '%s'",
            wt, output.length(), outpath.c_str());
        oError.append("Error saving output image");
        return std::string();
    }
    
    return outpath;
}
