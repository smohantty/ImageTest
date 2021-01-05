#include <iostream>
#include <memory>
#include <vector>
#include <dirent.h>
#include <algorithm>
#include <cstring>

#include <rlottie.h>
#include <thorvg.h>

#define PROJECT_NAME "ImageTest"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"


static constexpr int ImageWidth = 100;
static constexpr int ImageHeight = 100;

namespace Directory
{
    static bool jsonFile(const char *filename)
    {
        const char *dot = strrchr(filename, '.');
        if(!dot || dot == filename) return false;
        return !strcmp(dot + 1, "json");
    }

    static bool svgFile(const char *filename)
    {
        const char *dot = strrchr(filename, '.');
        if(!dot || dot == filename) return false;
        return !strcmp(dot + 1, "svg");
    }

    template<typename Filter>
    std::vector<std::string>
    ExtractFiles(const std::string &dirName, Filter filter)
    {
        DIR *d;
        struct dirent *dir;
        std::vector<std::string> result;
        d = opendir(dirName.c_str());
        if (d) {
          while ((dir = readdir(d)) != NULL) {
            if (filter(dir->d_name))
              result.push_back(dirName + dir->d_name);
          }
          closedir(d);
        }

        std::sort(result.begin(), result.end(), [](auto & a, auto &b){return a < b;});

        return result;
    }

    std::vector<std::string> lottieFiles(const std::string &dirName)
    {
        return ExtractFiles(dirName, jsonFile);
    }

    std::vector<std::string> svgFiles(const std::string &dirName)
    {
        return ExtractFiles(dirName, svgFile);
    }
};

namespace Utils
{
    static std::string basename(const std::string &str)
    {
        return str.substr(str.find_last_of("/\\") + 1);
    }
    void ARGBPremulToRGBA(rlottie::Surface &s)
    {
        uint8_t *buffer = reinterpret_cast<uint8_t *>(s.buffer());
        uint32_t totalBytes = s.height() * s.bytesPerLine();

        for (uint32_t i = 0; i < totalBytes; i += 4) {
           unsigned char a = buffer[i+3];
           // compute only if alpha is non zero
           if (a) {
               unsigned char r = buffer[i+2];
               unsigned char g = buffer[i+1];
               unsigned char b = buffer[i];

                if (a != 255) {  // un premultiply
                    r = (r * 255) / a;
                    g = (g * 255) / a;
                    b = (b * 255) / a;

                    buffer[i] = r;
                    buffer[i + 1] = g;
                    buffer[i + 2] = b;

                } else {
                    // only swizzle r and b
                    buffer[i] = r;
                    buffer[i + 2] = b;
                }
           }
        }
    }
};


enum class TestMode
{
    Generate,
    Test,
    Both
};

class LottieTest
{
public:
    LottieTest(TestMode mode)
    :mBuffer(new uint32_t[ImageWidth * ImageHeight])
    {
        // collect all the resourse
        auto list = Directory::lottieFiles(LOTTIE_RESOURCE_DIR);
        int FailedTestCase = 0;

        // generate baseline image.
        if (mode == TestMode::Both || mode == TestMode::Generate) {
            std::cout<<"\n *** Genearting BaseLine Image for Lottie.. ***\n\n";
            for (auto &path : list) {
                std::cout<<Utils::basename(path)<<"\n";
                GenerateBaselineImage(path, ImageWidth, ImageHeight);
            }
            std::cout<<"\n *** BaseLine Image Generation for Lottie Done. ***\n\n";
        }
        if(mode == TestMode::Both || mode == TestMode::Test) {
            std::cout<<"\n *** Test Started for Lottie Total TestCase : "<<list.size()<<" *** \n\n";
            for (auto &path : list) {
                std::cout<<"TestCase : "<< Utils::basename(path)<< " Status : ";
                auto result = TestAgainstBaselineImage(path, ImageWidth, ImageHeight);
                if (!result.empty()) {
                    ++FailedTestCase;
                    std::cout<<"FAIL\n";
                    std::cout<<"\t Frames : [ ";
                    for(auto frameno : result){
                        std::cout<<frameno <<" ";
                    }
                    std::cout<<"\n";
                } else {
                    std::cout<<"PASS\n";
                }
            }
            std::cout<<"\n Total TestCase : "<<list.size()<<" \t Passed : "<<list.size() - FailedTestCase<< " \t Failed : "<< FailedTestCase <<"\n\n";
        }
    }
    void GenerateBaselineImage(const std::string& fileName, int w, int h)
    {
        auto player = rlottie::Animation::loadFromFile(fileName);
        if (!player) return;

        size_t frameCount = player->totalFrame();

        char Buffer[100];
        for (auto i = 0u; i < frameCount ; i++) {

            rlottie::Surface surface(mBuffer.get(), w, h, w * 4);
            player->renderSync(i, surface);

            Utils::ARGBPremulToRGBA(surface);

            snprintf(Buffer, sizeof(Buffer), ".%d.png", i);
            std::string resultFile = LOTTIE_BASELINE_DIR + Utils::basename(fileName) + std::string(Buffer);

            stbi_write_png(resultFile.c_str(), w, h, 4, mBuffer.get(), w * 4);
        }
    }

    std::vector<int> TestAgainstBaselineImage(const std::string& fileName, int w, int h)
    {
        std::vector<int> result;
        auto player = rlottie::Animation::loadFromFile(fileName);
        if (!player) return {1};

        size_t frameCount = player->totalFrame();

        char Buffer[100];
        for (auto i = 0u; i < frameCount ; i++) {

            rlottie::Surface surface(mBuffer.get(), w, h, w * 4);
            player->renderSync(i, surface);

            Utils::ARGBPremulToRGBA(surface);

            snprintf(Buffer, sizeof(Buffer), ".%d.png", i);
            std::string resultFile = LOTTIE_BASELINE_DIR + Utils::basename(fileName) + std::string(Buffer);

            int iw = 0, ih = 0, channels = 0;
            unsigned char* imageBuffer = stbi_load(resultFile.c_str(), &iw, &ih, &channels, 4);

            if (iw != w || ih != h || channels!= 4 || imageBuffer ==nullptr || memcmp(mBuffer.get(), imageBuffer, iw * ih * 4))
            {
                result.push_back(i);
            }
            stbi_image_free(imageBuffer);
        }
        return result;
    }

private:
    std::unique_ptr<uint32_t[]> mBuffer;
};

class SVGTest
{
public:
    SVGTest(TestMode mode)
    :mBuffer(new uint32_t[ImageWidth * ImageHeight])
    {
        tvg::Initializer::init(tvg::CanvasEngine::Sw, std::thread::hardware_concurrency());
        mSwCanvas = tvg::SwCanvas::gen();
        mSwCanvas->target(mBuffer.get(), ImageWidth, ImageWidth, ImageHeight, tvg::SwCanvas::ABGR8888);

        // collect all the resourse
        auto list = Directory::svgFiles(SVG_RESOURCE_DIR);
        int FailedTestCase = 0;

        // generate baseline image.
        if (mode == TestMode::Both || mode == TestMode::Generate) {
            std::cout<<"\n *** Genearting BaseLine Image for SVG.. ***\n\n";
            for (auto &path : list) {
                mSwCanvas->clear();
                std::cout<<Utils::basename(path)<<"\n";
                GenerateBaselineImage(path, ImageWidth, ImageHeight);
            }
            std::cout<<"\n *** BaseLine Image Generation for SVG Done. ***\n\n";
        }
        if(mode == TestMode::Both || mode == TestMode::Test) {
            std::cout<<"\n *** Test Started for SVG Total TestCase : "<<list.size()<<" *** \n\n";
            for (auto &path : list) {
                mSwCanvas->clear();
                std::cout<<"TestCase : "<< Utils::basename(path)<< " Status : ";
                auto result = TestAgainstBaselineImage(path, ImageWidth, ImageHeight);
                if (!result) {
                    ++FailedTestCase;
                    std::cout<<"FAIL\n";
                } else {
                    std::cout<<"PASS\n";
                }
            }
            std::cout<<"\n Total TestCase : "<<list.size()<<" \t Passed : "<<list.size() - FailedTestCase<< " \t Failed : "<< FailedTestCase <<"\n\n";
        }
    }

    void GenerateBaselineImage(const std::string& fileName, int w, int h)
    {
        auto picture = tvg::Picture::gen();

        if(picture->load(fileName) != tvg::Result::Success) return;

        picture->size(w, h);

        mSwCanvas->push(move(picture));

        if (mSwCanvas->draw() == tvg::Result::Success) {
            mSwCanvas->sync();
        }

        std::string resultFile = SVG_BASELINE_DIR + Utils::basename(fileName) + std::string(".png");

        stbi_write_png(resultFile.c_str(), w, h, 4, mBuffer.get(), w * 4);
    }

    bool TestAgainstBaselineImage(const std::string& fileName, int w, int h)
    {
        auto picture = tvg::Picture::gen();

        if(picture->load(fileName) != tvg::Result::Success) return false;

        picture->size(w, h);

        mSwCanvas->push(move(picture));

        if (mSwCanvas->draw() == tvg::Result::Success) {
            mSwCanvas->sync();
        }

        std::string resultFile = SVG_BASELINE_DIR + Utils::basename(fileName) + std::string(".png");

        stbi_write_png(resultFile.c_str(), w, h, 4, mBuffer.get(), w * 4);

        int iw = 0, ih = 0, channels = 0;
        unsigned char* imageBuffer = stbi_load(resultFile.c_str(), &iw, &ih, &channels, 4);

        if (iw != w || ih != h || channels!= 4 || imageBuffer ==nullptr || memcmp(mBuffer.get(), imageBuffer, iw * ih * 4))
        {
            return false;
        }
        stbi_image_free(imageBuffer);
        return true;
    }

private:
    std::unique_ptr<tvg::SwCanvas> mSwCanvas;
    std::unique_ptr<uint32_t[]> mBuffer;
};

class ArgParser
{
public:
    ArgParser(int argc, char **argv)
    {
        if (argc < 2) {
            mHelp = true;
            return;
        }

        int index = 1;
        while (index < argc) {
          const char* option = argv[index];
          index++;
          if (!strcmp(option,"--help") || !strcmp(option,"-h")) {
              mHelp = true;
              return;
          } else if (!strcmp(option,"-g")) {
              mGenerate = true;
          } else if (!strcmp(option,"-t")) {
             mTest = true;
          }
        }
    }

    TestMode testMode() const
    {
        if (mGenerate && mTest) {
            return TestMode::Both;
        }

        return mGenerate ? TestMode::Generate : TestMode::Test;
    }

    bool help() {
        if (mHelp) {
            std::cout<<"\n Usage: \n\tFor Generating Baseline Image : imagetest -g \n\n\tFor Testing : imagetest -t\n";
            std::cout<<"\n\timagetest -g [\"lottie\", \"svg\"] -t [\"lottie\", \"svg\"]\n\n";
        }

        return mHelp;
    }
private:
    bool mHelp{false};
    bool mGenerate{false};
    bool mTest{false};
};

int main(int argc, char **argv)
{
    ArgParser options(argc, argv);

    if (options.help()) return 0 ;

    LottieTest(options.testMode());
    SVGTest(options.testMode());
    return 0;
}
