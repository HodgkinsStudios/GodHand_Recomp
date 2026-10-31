#include "ps2_frame_capture.h"
#include <cassert>
#include <filesystem>
#include <vector>
#include <iostream>
int main(){
    auto dest=std::filesystem::temp_directory_path()/"cp03-guest-frame-test.ppm";
    std::vector<unsigned char> pixels(64*64*4,0);
    assert(!ps2x::captureNonUniformGuestFrame(pixels,64,64,dest));
    assert(!std::filesystem::exists(dest));
    for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){
        auto p=(y*64+x)*4;
        pixels[p]=static_cast<unsigned char>(x*3);
        pixels[p+1]=static_cast<unsigned char>(y*3);
        pixels[p+2]=100;
        pixels[p+3]=255;
    }
    assert(ps2x::captureNonUniformGuestFrame(pixels,64,64,dest));
    assert(std::filesystem::file_size(dest)==(size_t)(64*64*3+13));
    std::filesystem::remove(dest);
    std::cout<<"Framebuffer capture synthetic tests: PASS\n";
}
