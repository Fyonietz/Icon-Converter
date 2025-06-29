#include <iostream>
#include <windows.h>
#include <gdiplus.h>
#include <thread>
#include <sstream>
#include <string>
#include <filesystem>
#include "json.hpp"
#include <fstream>
#include <chrono>

class GDIPlusManager{
    public:
        GDIPlusManager(){
            Gdiplus::GdiplusStartupInput gdiplus_startup_input;
            GdiplusStartup(&gdiPlusToken,&gdiplus_startup_input,nullptr);
        };
        ~GDIPlusManager(){
            ULONG_PTR gdiplusToken;
        };

    private:
        ULONG_PTR gdiPlusToken;
};

std::wstring ConvertToWChar(const std::string& str){
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wideStr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wideStr[0], size_needed);
    return wideStr;
}


void get_icon(const char* input_path,const wchar_t  *output_path){
    GDIPlusManager gdiplus;

    std::filesystem::path p(input_path);

    HICON hIcon = nullptr;
    if(ExtractIconExA(input_path,0,&hIcon,nullptr,1) <= 0 || hIcon == nullptr){
        std::cerr << "Failed To Extract Icon" << std::endl;
        return;
    }

    Gdiplus::Bitmap *bmp = Gdiplus::Bitmap::FromHICON(hIcon);
    if(!bmp){
        std::cerr << "Failed To Convert Icon To Bitmap\n";
        DestroyIcon(hIcon);
        return;
    }

    CLSID clsid;
    UINT num = 0,size = 0;
    Gdiplus::GetImageEncodersSize(&num,&size);
    if(size == 0){
        std::cerr << "GetImageEncodersSize Failed.\n";
        delete bmp;
        DestroyIcon(hIcon);
        return;
    }
    Gdiplus::ImageCodecInfo *codecs = (Gdiplus::ImageCodecInfo*)(malloc(size));
    Gdiplus::GetImageEncoders(num,size,codecs);

    bool found = false;
    for(UINT i = 0;i<num;++i){
        if(wcsstr(codecs[i].FilenameExtension,L"*.PNG") != nullptr){
            clsid = codecs[i].Clsid;
            found = true;
            break;
        }
    }

    free(codecs);

    if(!found){
        std::cerr << "ICO Encoders Not Found.\n";
        delete bmp;
        DestroyIcon(hIcon);
        return;
    }
    if(bmp->Save(output_path,&clsid,nullptr) != Gdiplus::Ok){
        std::cerr << "Failed To Save ICO file.\n";
    }else {
        std::cout << "Icon Saved.\n";
    }
    delete bmp;
    DestroyIcon(hIcon);
};

struct AppInfo{
    std::string name;
    std::string app_image;
    std::string app_location;
};

std::vector<AppInfo> parse_app_lists(const std::string &filename){
    std::ifstream file(filename);
    nlohmann::json json_data;
    file >> json_data;

    std::vector<AppInfo> apps;

    for(const auto &item : json_data["app_lists"]){
        for(auto it = item.begin();it != item.end();++it){
            AppInfo app;
            app.name = it.key();
            app.app_image = it.value()["app_image"];
            app.app_location = it.value()["app_location"];
            apps.push_back(app);
        }
    }
    return apps;
};

void generate(std::vector<AppInfo> &apps,std::string &output_path,char *argv[]){
        int counter = 0;
        for (const auto &app: apps){
        output_path = std::string(argv[2]) +app.name+ ".png";
        std::wstring wide_output = ConvertToWChar(output_path);
        get_icon(app.app_location.c_str(), wide_output.c_str());
        std::cout << "App Extracted: " << ++counter << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
}
int main(int argc,char *argv[]){
    if(argc < 3){
        std::cout << "Use <input.json> <output_path>" << std::endl;
        return 0;
    }

    std::string input_path = argv[1];
    std::string output_path;

    if(input_path.size() >= 5 && input_path.substr(input_path.size() - 5) == ".json"){
        std::cout << "Json File Detected" << std::endl;
        std::vector<AppInfo> apps;
        try {
            apps = parse_app_lists(input_path);
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
            return 1;
        }
        if(apps.empty()) {
            std::cerr << "No apps found in JSON." << std::endl;
            return 1;
        }
        output_path = std::string(argv[2]) + apps[0].name + ".png";
        auto start = std::chrono::high_resolution_clock::now();
        std::thread worker(generate,std::ref(apps), std::ref(output_path), argv);
        worker.join();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end-start;
        std::cout << "Icon Extractions Done" << std::endl;
        std::cout << "Time Taken(s): " <<duration.count() << "Seconds" << std::endl;
    }else{
        std::cout << "Error : Must .JSON File Generated From Sea.exe" << std::endl;
        return 1;
    }
    

    return 0;
}