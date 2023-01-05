//
// Created by z on 2020/7/17.
//
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>


//#include "utils.h"
#include "RkMppEncoder.h"

using namespace std;



std::vector<char> ReadFile(const std::string filename)
{
	std::vector<char> buffer;
    std::ifstream is(filename, std::ios::binary | std::ios::ate);
	if (!is.is_open())
		return buffer; 
    is.unsetf(std::ios::skipws);

    std::streampos size;
    is.seekg(0, std::ios::end);
    size = is.tellg();
    is.seekg(0, std::ios::beg);

    
    buffer.reserve(size);

    buffer.insert(buffer.begin(), std::istream_iterator<char>(is), std::istream_iterator<char>());


    return buffer;
}

int main(int argc, char* argv[]){
    std::cout<<"hello"<<std::endl;
    osee::MppEncoder mppenc;
    mppenc.MppEncdoerInit(1920, 1080, 30);


    int count = 0;
    int length = 0;

    cout<<"main test 4"<<endl;
    char dst[1024*1024*4];
    char img[1024*1024*4];
    char *pdst = dst;
    FILE* fp = fopen("nv12.h264", "wb+");
#if 1
    std::vector<char> file = ReadFile("/tmp/out.yuv");
#else
    FILE* yuv = fopen("/tmp/out.yuv", "rb");
    if (yuv != NULL )
    {
        fread(img,1, 345600, yuv );
    }
    else{
        std::cout<<"read file failed"<<std::endl;
        return 0;
    }
#endif

	printf("read over!\n");
    while (count++ < 500)
    {
        std::cout<<count++<<std::endl;
		printf("file size=%d\n", file.size());
        //mppenc.encode(file.data(), file.size(), pdst, &length);
        //mppenc.encode(file.data(), 3110400, pdst, &length);
        // mppenc.encode(img, 345600, pdst, &length);
        fwrite(dst, length,1,  fp);
    }

    cout<<"main test 6"<<endl;
    fclose(fp);
    return 0;
}
