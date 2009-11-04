#ifndef __IMAGEPROCESSOR_H__
#define __IMAGEPROCESSOR_H__

#include <string>
#include "bptypeutil.hh"

namespace imageproc {
    
    typedef enum { UNKNOWN, JPEG, PNG, GIF, } Type;

    /** given a path, make a best guess at what kind of image is
     *  contained within */
    Type pathToType(const std::string & path); 

    /** given an image type, generate a reasonable
     *  contained within */
    std::string typeToExt(const std::string & typeStr);
    
    /** perform a series of operations on an image
     *  inPath - the path to an input image
     *  tmpdir - a directory where the result should be stored
     *  outputFormat - the type of image to return
     *  transformations - a list of transformations to perform in order
     *  error - a verbose developer readable english error
     *  \returns .empty() on error, otherwise the path to resulting image
     */ 
    std::string ChangeImage(    
        const std::string & inPath,
        const std::string & tmpdir,
        Type outputFormat,
        const bp::List & transformations,
        std::string & error);
};

#endif
