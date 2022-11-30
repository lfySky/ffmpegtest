#include <iostream>
#include "MP4Muxer.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
	MP4Muxer * pMP4Muxer = new MP4Muxer();
    pMP4Muxer->CreateMp4("./test.mp4");
	int quit = 1;
    while(!quit) {
        pMP4Muxer->WriteVideo(nullptr, 0, 0);
    }
    pMP4Muxer->CloseMp4();
    return 0;
}
