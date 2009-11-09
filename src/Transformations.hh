/*
 * A collection of image transformations
 */

#ifndef __TRANSFORMATIONS_H__
#define __TRANSFORMATIONS_H__

#include "service.hh"
#include "bptypeutil.hh"

#include <magick/api.h>
    
namespace trans {
    /**
     *  All image processing phases conform to this signature:
     */
    typedef Image * (*TransformationFunc)(const Image * inImage,
                                          const bp::Object * args,
                                          int quality, std::string &oError);

    typedef struct {
        // the name of the transformation (as a client would specify it)
        const char * name;
        // does this accept arguments?
        bool acceptsArgs;
        // does this require arguments?
        bool requiresArgs;
        // the function that actually performs work 
        TransformationFunc transform;
        // documentation
        const char * doc;
    } Transformation;

    unsigned int num();
    const Transformation * get(unsigned int);
    const Transformation * get(const std::string & name);
};

#endif
