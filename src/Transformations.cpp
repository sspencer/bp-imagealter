#include "Transformations.hh"

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

static trans::Transformation s_transMap[] = {
    { "noop", false, false, noopTransform }
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
