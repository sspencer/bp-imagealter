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

#ifndef __IMAGEPROCESSOR_HH__
#define __IMAGEPROCESSOR_HH__

#include <string>
#include "bptypeutil.hh"

namespace imageproc {
    
	/** once per process initialization */
	void init();

	/** once per process shutdown */
	void shutdown();

    // image type is a short text string
    typedef const char * Type;
    extern const Type UNKNOWN;

    /** given a path, make a best guess at what kind of image is
     *  contained within */
    Type pathToType(const std::string & path); 

    /** given an image type, generate a reasonable
     *  contained within */
    std::string typeToExt(Type t);
    
    /** perform a series of operations on an image
     *  inPath - the path to an input image
     *  tmpdir - a directory where the result should be stored
     *  outputFormat - the type of image to return (short string rep)
     *  transformations - a list of transformations to perform in order
     *  error - a verbose developer readable english error
     *  \returns .empty() on error, otherwise the path to resulting image
     */ 
    std::string ChangeImage(    
        const std::string & inPath,
        const std::string & tmpdir,
        Type outputFormat,
        const bp::List & transformations,
        int quality,
        std::string & error);
};

#endif
