#include "Transformations.hh"
#include "service.hh"

#include <sstream>

static Image * noopTransform(const Image * inImage,
                             const bp::Object * args,
                             int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    DestroyExceptionInfo(&exception);
    return i;
}


static Image * solarizeTransform(const Image * inImage,
                                 const bp::Object * args,
                                 int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!i) {
        oError.append("couldn't clone image :/");        
    } else if (!SolarizeImage(i, 1.0)) {
        oError.append("error during solarization occured");
        DestroyImage(i);
        i = NULL;
    }
    DestroyExceptionInfo(&exception);
    return i;
}

static Image * oilpaintTransform(const Image * inImage,
                                 const bp::Object * args,
                                 int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = OilPaintImage( inImage, 2.0, &exception );
    DestroyExceptionInfo(&exception);
    return i;
}


static Image * rotateTransform(const Image * inImage,
                               const bp::Object * args,
                               int quality, std::string &oError)
{
    double degrees = 90;
    if (args != NULL) {
        if (args->type() == BPTDouble) {
            degrees = (double) *args;
        } else if (args->type() == BPTInteger) {
            degrees = (double)((long long)(*args));
        } else {
            oError.append("rotate accepts a single optional numeric argument");
            return NULL;
        }
    }
    
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = RotateImage( inImage, degrees, &exception );
    DestroyExceptionInfo(&exception);
    return i;
}



static Image * scaleTransform(const Image * inImage,
                              const bp::Object * args,
                              int quality, std::string &oError)
{
    int maxwidth = -1;
    int maxheight = -1;
    
    assert(args != NULL);
    
    if (args->type() != BPTMap) {
        oError.append("rotate accepts an object containing one or more "
                      "of the properties: maxwidth, maxheight");
        return NULL;
    }

    bp::Map::Iterator i(*((const bp::Map *) args));
    const char * k;
    while (NULL != (k=i.nextKey())) {
        int * num = NULL;
        const bp::Object * v = args->get(k);
        if (!strcasecmp("maxwidth", k)) num = &maxwidth;
        else if (!strcasecmp("maxheight", k)) num = &maxheight;
        else {
            std::stringstream ss;
            ss << "invalid argument to scale: " << k;
            oError = ss.str();
            return NULL;
        }

        if (v->type() != BPTInteger) {
            std::stringstream ss;
            ss << k << " requires an integer argument";
            oError = ss.str();
            return NULL;
        }

        *num = (int)((long long) *v);
    }

    // wow.  parsing arguments is lame.  but we did it. maxheight and
    // maxwidth now contain values that we should constrain to
    
    // first we'll determine the size of the input
    unsigned int x = inImage->magick_columns;
    unsigned int y = inImage->magick_rows;
    unsigned int origx = x, origy = y;
    if (maxwidth <= 0) maxwidth = x;
    if (maxheight <= 0) maxheight = y;
    
    // squish 
    if (x > (unsigned int) maxwidth)
    {
        double scale = (double) maxwidth / (double) x;
        x *= scale;
        y *= scale;
    }

    // smush 
    if (y > (unsigned int) maxheight)
    {
        double scale = (double) maxheight / (double) y;
        x *= scale;
        y *= scale;
    }

    // log about it
    g_bpCoreFunctions->log(
        BP_INFO,
        "scaling parameters [mw: %d | mh: %d]: "
        "from (%lu, %lu) to (%lu, %lu)",
        maxwidth, maxheight, origx, origy, x, y);

    // XXX: should we use different scaling algos based on quality?

    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * img = ResizeImage(inImage, x, y, LanczosFilter, 1.0, &exception);
    DestroyExceptionInfo(&exception);

    return img;
}

static trans::Transformation s_transMap[] = {
    { "noop", false, false, noopTransform },
    { "solarize", false, false, solarizeTransform },
    { "rotate", true, false, rotateTransform },
    { "scale", true, true, scaleTransform },    
    { "oilpaint", true, true, oilpaintTransform }    
};

unsigned int
trans::num()
{
    return sizeof(s_transMap)/sizeof(s_transMap[0]);
}

const trans::Transformation *
trans::get(unsigned int i)
{
    return s_transMap + i;
}

#ifdef WIN32
#define strcasecmp _stricmp
#endif

const trans::Transformation *
trans::get(const std::string & name)
{
    for (unsigned int i = 0; i < num(); i++) {
        if (!strcasecmp(name.c_str(), get(i)->name)) return get(i);
    }
    return NULL;
}
