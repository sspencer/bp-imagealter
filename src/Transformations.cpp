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

static Image * contrastTransform(const Image * inImage,
                                 const bp::Object * args,
                                 int quality, std::string &oError)
{
    int contrast = 1;
    
    if (args) {
        if (args->type() != BPTInteger) {
            oError.append("contrast takes a single numeric argument between "
                          "-10 and 10");
            return NULL;
        }
        contrast = (int) ((long long) *args);
    }
    

    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!i) {
        oError.append("couldn't clone image :/");        
    } else {
        int sharpen = 1;
        if (contrast < 0) {
            sharpen = 0;
            contrast *= -1;
        }
        if (contrast > 10) contrast = 10;
        for (int x = 0; x < contrast; x++) {
            if (!ContrastImage(i, sharpen)) {
                oError.append("error during contrast occured");
                DestroyImage(i);
                i = NULL;
                break;
            }
        }
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


static Image * swirlTransform(const Image * inImage,
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
            oError.append("swirl accepts a single optional numeric argument");
            return NULL;
        }
    }
    
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = SwirlImage( inImage, degrees, &exception );
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


static Image * cropTransform(const Image * inImage,
                             const bp::Object * args,
                             int quality, std::string &oError)
{
    // first we'll validate and extract parameters
    double cropParams[4];
    assert(args != NULL);
    
    if (!args || args->type() != BPTList ||
        ((const bp::List *) args)->size() != 4)
    {
        oError.append("crop accepts an array of four floating point numbers");
        return NULL;
    }

    const bp::List * l = (const bp::List *) args;
    for (unsigned int i = 0; i < 4; i++)
    {
        if  (l->value(i)->type() != BPTDouble) {
            oError.append("crop accepts an array of four "
                          "floating point numbers");
            return NULL;
        }

        cropParams[i] = (double) *(l->value(i));
        if (cropParams[i] < 0.0) cropParams[i] = 0.0;
        if (cropParams[i] > 1.0) cropParams[i] = 1.0;
    }

    // validate arguments
    if (cropParams[0] >= cropParams[2] || cropParams[1] >= cropParams[3]) {
        oError.append("meaningless crop parameters (x1/y1 may not be greater"
                      " than x2/y2)");
        return NULL;
    }
    

    // cropParams now contains x1, y1, x2, y2 in relative cordinates,
    // with origin at top left of image.  We'll use that information to
    // populate a RectangleInfo structure

    // extract existing image size
    unsigned int x = inImage->magick_columns;
    unsigned int y = inImage->magick_rows;

    RectangleInfo ri;
    ri.height = y * (cropParams[3] - cropParams[1]);
    ri.width = x * (cropParams[2] - cropParams[0]);
    ri.x = x * cropParams[0];
    ri.y = y * cropParams[1];

    g_bpCoreFunctions->log(
        BP_INFO,
        "Cropping image (%lux%lu): %lux%lu starting at %lu,%lu",
        x, y, ri.width, ri.height, ri.x, ri.y);
    
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * img = CropImage( inImage, &ri, &exception );
    DestroyExceptionInfo(&exception);

    return img;
}


static Image * grayscaleTransform(const Image * inImage,
                                  const bp::Object * args,
                                  int quality, std::string &oError)
{
    ExceptionInfo exception;
    QuantizeInfo qi;
    GetExceptionInfo(&exception);
    GetQuantizeInfo(&qi);

    qi.colorspace = GRAYColorspace;

    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!i) {
        oError.append("couldn't clone image :/");        
    } else if (!QuantizeImage(&qi, i)) {
        oError.append("error during grayscale quantize phase occured");
        DestroyImage(i);
        i = NULL;
    }

//    DestroyQuantizeInfo(&qi);
    DestroyExceptionInfo(&exception);
    return i;
}

static Image * psychodelicTransform(const Image * inImage,
                                    const bp::Object * args,
                                    int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!i) {
        oError.append("couldn't clone image :/");        
    } else if (!CycleColormapImage(i, 8)) {
        oError.append("error during psychodlic occured");
        DestroyImage(i);
        i = NULL;
    }
    DestroyExceptionInfo(&exception);
    return i;
}


static Image * negateTransform(const Image * inImage,
                               const bp::Object * args,
                               int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!i) {
        oError.append("couldn't clone image :/");        
    } else if (!NegateImage(i, 0)) {
        oError.append("error during negate occured");
        DestroyImage(i);
        i = NULL;
    }
    DestroyExceptionInfo(&exception);
    return i;
}


static Image * sepiaTransform(const Image * inImage,
                              const bp::Object * args,
                              int quality, std::string &oError)
{
    ExceptionInfo exception;
    QuantizeInfo qi;
    GetExceptionInfo(&exception);
    GetQuantizeInfo(&qi);
    qi.colorspace = GRAYColorspace;
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);

    // sepia pixelpacket
    PixelPacket p;
    p.red = 112;
    p.green = 66;
    p.blue = 20;
    p.opacity = 0;

    if (!i) {
        oError.append("couldn't clone image :/");        
    } else if (!QuantizeImage(&qi, i)) {
        oError.append("error during sepia quanitzation occured");
        DestroyImage(i);
        i = NULL;
    } 
    
    if (i) {
        Image * colorized = ColorizeImage(i, "50%", p, &exception);
        DestroyImage(i);
        i = colorized;
        if (!i) oError.append("error during sepia colorization occured");
    }
                                          
//    DestroyQuantizeInfo(&qi);
    DestroyExceptionInfo(&exception);
    return i;
}


static trans::Transformation s_transMap[] = {
    { "contrast", true, false, contrastTransform },    
    { "crop", true, true, cropTransform },    
    { "grayscale", true, true, grayscaleTransform },    
    { "greyscale", true, true, grayscaleTransform },    
    { "negate", false, false, negateTransform },
    { "noop", false, false, noopTransform },
    { "oilpaint", true, true, oilpaintTransform },    
    { "psychodelic", false, false, psychodelicTransform },
    { "rotate", true, false, rotateTransform },
    { "scale", true, true, scaleTransform },    
    { "sepia", true, true, sepiaTransform },    
    { "solarize", false, false, solarizeTransform },
    { "swirl", true, true, swirlTransform }    
    
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
