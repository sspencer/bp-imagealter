#include "Transformations.hh"
#include "service.hh"

#include <sstream>

#include <assert.h>

#ifdef WIN32
#define strcasecmp _stricmp
#endif

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


static Image * blurTransform(const Image * inImage,
                             const bp::Object * args,
                             int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = BlurImage(inImage, 1.0, 0.5, &exception);
    DestroyExceptionInfo(&exception);
    return i;
}

static Image * sharpenTransform(const Image * inImage,
                             const bp::Object * args,
                             int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = SharpenImage(inImage, 2.0, 1.0, &exception);
    DestroyExceptionInfo(&exception);
    return i;
}

static Image * unsharpenTransform(const Image * inImage,
                             const bp::Object * args,
                             int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = UnsharpMaskImage( inImage, 0, 0.5, 1, 0.05, &exception );
    DestroyExceptionInfo(&exception);
    return i;
}


static Image * despeckleTransform(const Image * inImage,
                                  const bp::Object * args,
                                  int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = DespeckleImage(inImage, &exception);
    DestroyExceptionInfo(&exception);
    return i;
}


static Image * enhanceTransform(const Image * inImage,
                                const bp::Object * args,
                                int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = EnhanceImage(inImage, &exception);
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


static bool
extractScalingDimensions(const char * funcName,
                         const Image * inImage,                         
                         const bp::Object * args,
                         unsigned int &x,
                         unsigned int &y,                         
                         std::string &oError)
{
    x = y = 0;
    int maxwidth = -1;
    int maxheight = -1;
    
    assert(args != NULL);
    
    if (args->type() != BPTMap) {
        oError.append(funcName);
        oError.append(" accepts an object containing one or more "
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
            ss << "invalid argument to " << funcName << ": " << k;
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
    x = inImage->columns;
    y = inImage->rows;
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

    return true;
}

static Image * scaleTransform(const Image * inImage,
                              const bp::Object * args,
                              int quality, std::string &oError)
{
    unsigned int x = 0, y = 0;
    
    if (!extractScalingDimensions("scale", inImage, args, x, y, oError))
    {
        return NULL;
    }
    
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * img = ResizeImage(inImage, x, y, LanczosFilter, 1.0, &exception);
    DestroyExceptionInfo(&exception);

    return img;
}

static Image * thumbnailTransform(const Image * inImage,
                                   const bp::Object * args,
                                   int quality, std::string &oError)
{
    unsigned int x = 0, y = 0;
    
    if (!extractScalingDimensions("thumbnail", inImage, args, x, y, oError))
    {
        return NULL;
    }
    
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * img = ThumbnailImage(inImage, x, y, &exception);
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


static Image * equalizeTransform(const Image * inImage,
                                  const bp::Object * args,
                                  int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!i) {
        oError.append("couldn't clone image :/");        
    } else if (!EqualizeImage(i)) {
        oError.append("error occured during equalization");
        DestroyImage(i);
        i = NULL;
    }
    DestroyExceptionInfo(&exception);
    return i;
}

static Image * normalizeTransform(const Image * inImage,
                                  const bp::Object * args,
                                  int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!i) {
        oError.append("couldn't clone image :/");        
    } else if (!NormalizeImage(i)) {
        oError.append("error occured during normalization");
        DestroyImage(i);
        i = NULL;
    }
    DestroyExceptionInfo(&exception);
    return i;
}


static Image * ditherTransform(const Image * inImage,
                               const bp::Object * args,
                               int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!i) {
        oError.append("couldn't clone image :/");        
    } else if (!OrderedDitherImage(i)) {
        oError.append("error occured during ditherizasification");
        DestroyImage(i);
        i = NULL;
    }
    DestroyExceptionInfo(&exception);
    return i;
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

static Image * psychedelicTransform(const Image * inImage,
                                    const bp::Object * args,
                                    int quality, std::string &oError)
{
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!i) {
        oError.append("couldn't clone image :/");        
    } else if (!CycleColormapImage(i, 8)) {
        oError.append("error during psychedlic occured");
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


static MagickPassFail sepiaWorker(
	void *mutable_data,         /* User provided mutable data */
	const void *immutable_data, /* User provided immutable data */
	Image *image,               /* Modify image */
	PixelPacket *pixels,        /* Pixel row */
	IndexPacket *indexes,       /* Pixel row indexes */
	const long npixels,         /* Number of pixels in row */
	ExceptionInfo *exception)   /* Exception report */
{
  	register long i;  

	float r1, g1, b1, r2, g2, b2;

	// Modified version of this:
	// http://blogs.techrepublic.com.com/howdoi/?p=120
  	for (i=0; i < npixels; i++) {
		r1 = (float)pixels[i].red;
		g1 = (float)pixels[i].green;
		b1 = (float)pixels[i].blue;

		r2 = (r1 * 0.373 + g1 * 0.731 + b1 * 0.180);
		g2 = (r1 * 0.298 + g1 * 0.586 + b1 * 0.143);
		b2 = (r1 * 0.219 + g1 * 0.431 + b1 * 0.105);

		if (r2 > MaxRGB) r2 = MaxRGB;
		if (g2 > MaxRGB) g2 = MaxRGB;
		if (b2 > MaxRGB) b2 = MaxRGB;

		pixels[i].red   = r2;
		pixels[i].green = g2;
		pixels[i].blue  = b2;
	}
  
	return MagickPass;
}

static Image * sepiaTransform(const Image * inImage,
                              const bp::Object * args,
                              int quality, std::string &oError)
{
	MagickPassFail status=MagickPass;
    ExceptionInfo exception;
    GetExceptionInfo(&exception);
    Image * i = CloneImage(inImage, 0, 0, 1, &exception);

    if (!i) {
        oError.append("couldn't clone image :/");        
	} else {
		status=PixelIterateMonoModify(
			sepiaWorker,
			NULL, // const PixelIteratorOptions *options
			NULL, // const char *description
			NULL, // void *mutable_data
			NULL, // const void *immutable_data
			0, // const long x
			0, // const long y
			i->columns, // const unsigned long columns, 
			i->rows, // const unsigned long rows, 
			i,
			&exception);
		
		if (status == MagickFail) {
        	oError.append("error during sepia quanitzation occured");
        	DestroyImage(i);
        	i = NULL;
		} else {
			// Add some little contrast to sepia toned image.  Looks much better with contrast.
			i = contrastTransform(i, NULL, 100, oError);
		}
	}

    DestroyExceptionInfo(&exception);
    return i;
}


static Image * thresholdTransform(const Image * inImage,
                                  const bp::Object * args,
                                  int quality, std::string &oError)
{
    double threshold = 128.0;
    if (args != NULL) {
        if (args->type() == BPTDouble) {
            threshold = (double) *args;
        } else if (args->type() == BPTInteger) {
            threshold = (double)((long long)(*args));
        } else {
            oError.append("threshold accepts a single optional numeric argument");
            return NULL;
        }
    }
    if (threshold < 0.0) threshold = 0.0;
    if (threshold > 256.0) threshold = 256.0;

    ExceptionInfo exception;
    GetExceptionInfo(&exception);

    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if ( !ThresholdImage( i, threshold ) )
    {
        DestroyImage(i);        
        i = NULL;
    }
    DestroyExceptionInfo(&exception);
    return i;
}



static Image * blackThresholdTransform(const Image * inImage,
                                       const bp::Object * args,
                                       int quality, std::string &oError)
{
    double threshold = 50.0;
    if (args != NULL) {
        if (args->type() == BPTDouble) {
            threshold = (double) *args;
        } else if (args->type() == BPTInteger) {
            threshold = (double)((long long)(*args));
        } else {
            oError.append("black_threshold accepts a single optional numeric argument");
            return NULL;
        }
    }
    if (threshold < 0.0) threshold = 0.0;
    if (threshold > 100.0) threshold = 100.0;

    char thresholdString[10];
    sprintf(thresholdString, "%d%%", (int) threshold);
    
    ExceptionInfo exception;
    GetExceptionInfo(&exception);

    Image * i = CloneImage(inImage, 0, 0, 1, &exception);
    if (!BlackThresholdImage( i, thresholdString ))
    {
        DestroyImage(i);        
        i = NULL;
    }
    DestroyExceptionInfo(&exception);
    return i;
}



static trans::Transformation s_transMap[] = {
    {
        "contrast", true, false, contrastTransform,
        "adjust the image's contrast, accepts an optional numeric argument "
        "between -10 and 10"
    },    
    {
        "black_threshold", true, false, blackThresholdTransform,
        "Given a threshold (in terms of percentage from 0-100), color all "
        "pixels which fall under that threshold black."
    },
    {
        "blur", false, false, blurTransform,
        "blur (or 'smooth') an image"
    },    
    {
        "crop", true, true, cropTransform,
        "select a subset of an image, accepts an array of four floating point "
        "numbers: x1,y1,x2,y2 which are between 0.0 and 1.0 and are relative "
        "coordinates to the upper left hand corner of the image"
    },    
    {
        "despeckle", false, false, despeckleTransform,
        "reduces the speckle noise in an image while perserving the edges of "
        "the original image, accepts no arguments"
    },
    {
        "dither", false, false, ditherTransform,
        "Uses the ordered dithering technique of reducing color images to monochrome using positional information to retain as much information as possible."
    },
    {
        "enhance", false, false, enhanceTransform,
        "Applies a digital filter that improves the quality of a noisy image, "
        "accepts no arguments "
    },    
    {
        "equalize", false, false, equalizeTransform,
        "Applies a histogram equalization to the image."
    },

    {
        "grayscale", false, false, grayscaleTransform,
        "remove the color from an image, accepts no arguments"
    },    
    {
        "greyscale", true, true, grayscaleTransform,
        "an alias for 'grayscale'"
    },    
    {
        "negate", false, false, negateTransform,
        "negate the colors of the image, accepts no arguments"
    },
    {
        "noop", false, false, noopTransform,
        "do nothing.  may be applied multiple times.  still does nothing."
    },
    {
        "normalize", false, false, normalizeTransform,
        "Enhances the contrast of a color image by adjusting the pixels color to span the entire range of colors available."
    },
    {
        "oilpaint", false, false, oilpaintTransform,
        "an effect that will make the image look like an oil painting, "
        "accepts no arguments"
    },    
    {
        "psychedelic", false, false, psychedelicTransform,
        "trip out an image.  takes no arguments.  may be applied multiple "
        "times."
    },
    {
        "rotate", true, false, rotateTransform,
        "rotate an image by some number of degrees, takes a single numeric "
        "argument"
    },
    {
        "scale", true, true, scaleTransform,
        "downscale an image preserving aspect ratio.  you may provide the "
        "integer arguments maxwidth and/or maxheight which limit the image "
        "in the specified direction.  units are pixels."
    },    
    {
        "sepia", false, false, sepiaTransform,
        "sepia tone an image.  no arguments."
    },    
    {
        "sharpen", false, false, sharpenTransform,
        "sharpen an image"
    },    
    {
        "solarize", false, false, solarizeTransform,
        "solarize an image.  no arguments"
    },
    {
        "swirl", true, true, swirlTransform,
        "swirl an image.  optionally a numeric argument specifies the degrees "
        "to swirl, default is 90 degrees."
    },
    {
        "threshold", true, false, thresholdTransform,
        "given a numeric threshold collapse pixels of intensity greater than "
        "the threshold to white, and those less than to black.  Result is a "
        "two color image.  Accepts a single numeric arg from 0-256, default "
        "is 128."
    },
    {
        "thumbnail", true, true, thumbnailTransform,
        "An alternate version of 'scale' optimized for fast thumnailing, "
        "combine with a relatively high 'quality' argument (75-85) for "
        "the best balance between speed and quality.  Accepts the same "
        "arguments as 'scale'."
    },    
    {
        "unsharpen", false, false, unsharpenTransform,
        "unsharpen an image"
    }
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