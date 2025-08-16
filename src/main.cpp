#include <iostream>
#include "../pch.h"
#include "../headers/Loader.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../headers/stb_image_write.h"

int main()
{

    try
    {
        std::string _current_path = std::filesystem::current_path().string();
        LanternImageLoader<10, 200, 200, true> loader;
        loader.CreateDatasetForFolder("trains");
        loader.SelectDatasetToModify("trains");
        loader.GetImagesDataFromFolder(_current_path + "/../dataset/cats");
        loader.GetImagesDataFromFolder(_current_path + "/../dataset/dogs");
        loader.ReadCSVLabelDataFromFolder(_current_path + "/../dataset/labels.csv");
        loader.Run();

        lantern::utility::Vector<std::string> label;
        lantern::utility::Vector<af::array> img_data;
        for (uint32_t i = 0; i < 10; i++)
        {
            img_data.push_back(af::array(200, 200, 3, af::dtype::u8));
            label.push_back("");
        }

        af::Window app(1200,700,"Lantern Image Loader");
        app.grid(2,5);
        while(true){
            for (uint32_t i = 0; i < 10; i++)
            {
                loader.GetaAsAF(img_data[i],label[i]);
            }
            do
            {
                app(0,0).image(img_data[0], (label[0] + " Image ").c_str());
                app(0,1).image(img_data[1], (label[1] + " Image ").c_str());
                app(0,2).image(img_data[2], (label[2] + " Image ").c_str());
                app(0,3).image(img_data[3], (label[3] + " Image ").c_str());
                app(0,4).image(img_data[4], (label[4] + " Image ").c_str());
                app(1,0).image(img_data[5], (label[5] + " Image ").c_str());
                app(1,1).image(img_data[6], (label[6] + " Image ").c_str());
                app(1,2).image(img_data[7], (label[7] + " Image ").c_str());
                app(1,3).image(img_data[8], (label[8] + " Image ").c_str());
                app(1,4).image(img_data[9], (label[9] + " Image ").c_str());
                app.show();
    
            } while (!app.close());
            std::cin.get();
        }

        loader.Stop();
    }
    catch (std::exception &err)
    {
        std::cout << err.what() << '\n';
    }

    return 0;
}