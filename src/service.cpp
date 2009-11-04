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

#include <ServiceAPI/bperror.h>
#include <ServiceAPI/bptypes.h>
#include <ServiceAPI/bpdefinition.h>
#include <ServiceAPI/bpcfunctions.h>
#include <ServiceAPI/bppfunctions.h>

#include "bptypeutil.hh"
#include "bpurlutil.hh"

#include "util/bpsync.hh"
#include "util/bpthread.hh"
#include "util/fileutil.hh"

#include "ImageProcessor.hh"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fstream>

#include <iostream>
#include <list>

const BPCFunctionTable * g_bpCoreFunctions = NULL;

struct SessionData {
    std::string tempDir;
};


static int
BPPAllocate(void ** instance, unsigned int, const BPElement * context)
{
    SessionData * sd = new SessionData;

    // extract the temporary directory
    bp::Object * args = bp::Object::build(context);
    sd->tempDir = (std::string) (*(args->get("temp_dir")));
    delete args;

    (void) ft::mkdir(sd->tempDir);
    g_bpCoreFunctions->log(BP_INFO, "session allocated, using temp dir: %s",
                           (sd->tempDir.empty() ? "<empty>"
                                                : sd->tempDir.c_str()));

    *instance = (void *) sd;
    return 0;
}

static void
BPPDestroy(void * instance)
{
    assert(instance != NULL);
    SessionData * sd = (SessionData *) instance;
    delete sd;
}

static void
BPPShutdown(void)
{
}

static void
BPPInvoke(void * instance, const char * funcName,
          unsigned int tid, const BPElement * elem)
{
    assert(instance != NULL);
    SessionData * sd = (SessionData *) instance;

    // confirm that they invoked the only function we got
    if (0 != strcmp(funcName, "transform"))
    {
        g_bpCoreFunctions->log(BP_ERROR, "invalid function invoked!");
        g_bpCoreFunctions->postError(
            tid, "bp.internalError",
            "unknown function invoked");
        return;
    }

    // XXX: we need to get a little thready here
    
    bp::Object * args = NULL;
    if (elem) args = bp::Object::build(elem);

    // first we'll get the input file into a string
    std::string url = (*(args->get("file")));
    std::string path = bp::urlutil::pathFromURL(url);

    if (path.empty())
    {
        g_bpCoreFunctions->log(
            BP_ERROR, "can't parse file:// url: %s", url.c_str());
        g_bpCoreFunctions->postError(
            tid, "bp.fileAccessError", "invalid file URI");
        if (args) delete args;
        return;
    } 
    
    // now let's figure out the output format
    imageproc::Type t = imageproc::UNKNOWN;
    if (args->has("format")) {
        t = imageproc::pathToType(*(args->get("format")));
    } else {
        t = imageproc::pathToType(path);
    }

    if (t == imageproc::UNKNOWN)
    {
        g_bpCoreFunctions->log(
            BP_ERROR, "can't determine output format");
        g_bpCoreFunctions->postError(
            tid, "bp.invalidArguments", "can't determine output format");
        if (args) delete args;
        return;
    } 

    // finally, let's pull out the list of transformation actions
    bp::List emptyList;
    bp::List * lPtr = &emptyList;
    
    if (args->has("actions")) lPtr = (bp::List *) args->get("actions");

    std::string err;
    
    std::string rez =
        imageproc::ChangeImage(path, sd->tempDir, t, *lPtr, err);
    
    if (rez.empty())
    {
        if (err.empty()) err.append("unknown");
        // error!
        g_bpCoreFunctions->log(
            BP_ERROR, "couldn't transform image: %s", err.c_str());
        g_bpCoreFunctions->postError(
            tid, "bp.transformFailed", err.c_str());
    }
    else
    {
        // success!
        bp::Map m;
        m.add("file", new bp::Path(rez));
        g_bpCoreFunctions->postResults(tid, m.elemPtr());
    }
    
    if (args) delete args;
    args = NULL;

    return;
}

BPArgumentDefinition s_transformFuncArgs[] = {
    {
        "file",
        "The image to transform.",
        BPTPath,
        BP_TRUE
    },
    {
        "format",
        "The format of the output image.  Default is to output in the same "
        "format as the input image.",
        BPTString,
        BP_FALSE
    },
    {
        "actions",
        "An array of transformations to perform on the input image.",
        BPTList,
        BP_FALSE
    }
};


static BPFunctionDefinition s_functions[] = {
    {
        "transform",
        "Perform a set of transformations on an input image.",
        sizeof(s_transformFuncArgs)/sizeof(s_transformFuncArgs[0]),
        s_transformFuncArgs
    }
};

const BPCoreletDefinition *
BPPInitialize(const BPCFunctionTable * bpCoreFunctions,
              const BPElement * parameterMap)
{
    // a description of this service
    static BPCoreletDefinition s_serviceDef = {
        "ImageAlter",
        4, 0, 0,
        "Implements client side Image manipulation",
        sizeof(s_functions)/sizeof(s_functions[0]),
        s_functions
    };

    g_bpCoreFunctions = bpCoreFunctions;
    return &s_serviceDef;
}

/** and finally, declare the entry point to the service */
BPPFunctionTable funcTable = {
    BPP_CORELET_API_VERSION,
    BPPInitialize,
    BPPShutdown,
    BPPAllocate,
    BPPDestroy,
    BPPInvoke,
    NULL,
    NULL
};

const BPPFunctionTable *
BPPGetEntryPoints(void)
{
    return &funcTable;
}
