#include "ImageProcessor.hh"

imageproc::Type
imageproc::pathToType(const std::string & path)
{
    return UNKNOWN;
}

std::string
imageproc::typeToExt(const std::string & typeStr)
{
    return std::string();
}

std::string
imageproc::ChangeImage(const std::string & inPath,
                       const std::string & tmpdir,
                       Type outputFormat,
                       const bp::List & transformations,
                       std::string & error)
{
    return std::string();
}
