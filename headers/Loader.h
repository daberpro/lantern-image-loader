#pragma once
#include "../pch.h"
#include "Vector.h"
#include "DataProcessing.h"
#include "File.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

template <uint32_t TOTAL_IMAGES, uint32_t IMG_WIDTH, uint32_t IMG_HEIGHT, bool IsColor>
class LanternImageLoader
{
private:
    std::mutex mutex;
    std::condition_variable producer, consumer;

    lantern::utility::Vector<uint32_t> each_class_sizes;
    std::unordered_map<std::string, lantern::utility::Vector<std::string>> image_paths;
    std::unordered_map<std::string, lantern::utility::Vector<uint8_t>> image_cache;
    std::unordered_map<std::string, std::array<std::string,TOTAL_IMAGES>> label_cache;
    std::unordered_map<std::string, CSVFile> labels;
    std::string active_dataset;
    size_t allocation = TOTAL_IMAGES * IMG_WIDTH * IMG_HEIGHT * (IsColor ? 3 : 1);
    std::thread thread_loader;
    std::atomic<bool> stop_thread = false;

    uint32_t head = 0, tail = 0, count = 0;

    void Put(const std::string &image_path)
    {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->producer.wait(lock, [this](){ return this->count < TOTAL_IMAGES || this->stop_thread; });
        if (this->stop_thread)
        {
            return;
        }
        this->CheckDatasetValid();
        auto &image_data = this->image_cache[this->active_dataset];
        auto &label_data = this->label_cache[this->active_dataset];
        int width, height, channels;
        stbir_pixel_layout layout = IsColor? STBIR_RGB : STBIR_1CHANNEL;
        uint8_t* image = stbi_load(image_path.c_str(), &width, &height, &channels, IsColor ? 3 : 1);
        if (!image) {
            std::println("Error LanternImageLoader, STB cannot load image \"{}\" because {}", image_path, stbi_failure_reason());
            return;
        }
        try {
            stbir_resize_uint8_linear(
                image,
                width, height, 0,
                image_data.getData() + (size_t)(this->tail * IMG_WIDTH * IMG_HEIGHT * (IsColor ? 3 : 1)),
                IMG_WIDTH, IMG_HEIGHT, 0,
                layout
            );
            
            label_data[this->tail] = std::filesystem::path(image_path).parent_path().filename().string();

        } catch (...) {
            stbi_image_free(image);
            return;
        }
        stbi_image_free(image);
        this->tail = (this->tail + 1) % TOTAL_IMAGES;
        this->count++;
        this->consumer.notify_all();
    }

    void Loaders()
    {

        this->each_class_sizes.back() -= 1;
        lantern::utility::Vector<uint32_t> batch_indices;
        uint32_t total_size_of_class = 0;
        for (auto &size : this->each_class_sizes)
        {
            total_size_of_class += size;
        }
        if (total_size_of_class == 0)
        {
            throw std::runtime_error("Error LanternImageLoader, No image found in dataset");
        }

        while (!this->stop_thread)
        {
            lantern::data::GetRandomSampleClassIndex<TOTAL_IMAGES>(batch_indices, this->each_class_sizes, total_size_of_class);
            for (auto &index : batch_indices)
            {
                if (this->stop_thread)
                {
                    return;
                }
                auto &_image_paths = this->image_paths[this->active_dataset];
                this->Put(_image_paths[index]);
            }
        }
    }

    std::unordered_set<std::string> extension_accepted = {
        ".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".webp"
    };
    /**
     * @brief Check if the file extension was an image or not
     * @param path 
     * @return bool
     */
    bool IsImage(const std::filesystem::path& path){

        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
        return this->extension_accepted.find(ext) != this->extension_accepted.end();

    }

public:
    LanternImageLoader() = default;

    uint8_t *Get()
    {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->consumer.wait(lock, [this](){ return this->count > 0 || this->stop_thread; });
        if (this->stop_thread || this->count == 0)
        {
            return nullptr; // Stop the thread if requested
        }
        auto &image_data = this->image_cache[this->active_dataset];
        uint8_t *image = image_data.getData() + (size_t)(this->head * IMG_WIDTH * IMG_HEIGHT * (IsColor ? 3 : 1));
        this->head = (this->head + 1) % TOTAL_IMAGES;
        this->count--;
        this->producer.notify_all();
        return image;
    }

    void CheckDatasetValid()
    {
        if (this->active_dataset.empty())
        {
            throw std::runtime_error("Error LanternImageLoader, No dataset selected");
        }
    }

    void GetImagesDataFromFolder(const std::filesystem::path &_path)
    {
        this->CheckDatasetValid();
        if (std::filesystem::exists(_path) && std::filesystem::is_directory(_path))
        {
            auto &image_paths = this->image_paths[this->active_dataset];
            uint32_t class_size = 0;
            for (auto &file : std::filesystem::directory_iterator(_path))
            {
                if (this->IsImage(file))
                {
                    image_paths.push_back(file.path().string());
                    class_size++;
                };
            }
            this->each_class_sizes.push_back(class_size);
        }
        else
        {
            throw std::runtime_error(std::format("Error ImageLoader, cannot access folder path \"{}\" looks like deleted or moved", _path.string()));
        }
    }

    void SelectDatasetToModify(const std::string &_dataset_name)
    {
        if (!this->image_cache.contains(_dataset_name))
        {
            throw std::runtime_error(std::format("Error LanternImageLoader, dataset \"{}\" do not exists", _dataset_name));
        }
        this->active_dataset = _dataset_name;
    }

    void CreateDatasetForFolder(const std::string &_dataset_name)
    {
        if (this->image_cache.contains(_dataset_name))
        {
            throw std::runtime_error(std::format("Error LanternImageLoader, Cannot create dataset \"{}\" because already exists", _dataset_name));
        }
        this->image_cache[_dataset_name] = lantern::utility::Vector<uint8_t>(this->allocation);
    }

    void Run()
    {
        this->thread_loader = std::thread(&LanternImageLoader::Loaders, this);
    }

    void Stop()
    {
        {
            std::lock_guard<std::mutex> lock(this->mutex);
            this->stop_thread = true;
        }
        this->producer.notify_all(); // Notify the producer to stop waiting
        this->consumer.notify_all();
        this->thread_loader.join();
    }

    /**
     * @brief Get CSV file from folder
     * @param _path 
     */
    void ReadCSVLabelDataFromFolder(const std::filesystem::path& _path) {
        this->CheckDatasetValid();
        this->labels[this->active_dataset] = ReadCSVFile(_path);
    }

    void GetAsAF(af::array &img){
        this->CheckDatasetValid();
        af::array flat(IMG_HEIGHT * IMG_WIDTH* 3, this->Get());
        af::array R = af::moddims(flat(af::seq(0, af::end, 3)), IMG_HEIGHT, IMG_WIDTH);
        af::array G = af::moddims(flat(af::seq(1, af::end, 3)), IMG_HEIGHT, IMG_WIDTH);
        af::array B = af::moddims(flat(af::seq(2, af::end, 3)), IMG_HEIGHT, IMG_WIDTH);

        img = af::join(2, R, G, B);
        img = af::reorder(img, 1, 0, 2);
        img = img.as(f32) / 255;
    }

    void GetAsAF(af::array &img, std::string &label){
        this->CheckDatasetValid();
        af::array flat(IMG_HEIGHT * IMG_WIDTH* 3, this->Get());
        af::array R = af::moddims(flat(af::seq(0, af::end, 3)), IMG_HEIGHT, IMG_WIDTH);
        af::array G = af::moddims(flat(af::seq(1, af::end, 3)), IMG_HEIGHT, IMG_WIDTH);
        af::array B = af::moddims(flat(af::seq(2, af::end, 3)), IMG_HEIGHT, IMG_WIDTH);

        img = af::join(2, R, G, B);
        img = af::reorder(img, 1, 0, 2);
        img = img.as(f32) / 255;

        auto& label_data = this->label_cache[this->active_dataset];
        label = label_data.at(this->head > 0? this->head - 1 : TOTAL_IMAGES - 1);
    }

    template <typename T>
    auto GetCSVLabelAtRow(const uint32_t& _row){
        this->CheckDatasetValid();
        return this->labels[this->active_dataset].Row<T>(_row);
    }

    template <typename T>
    auto GetCSVLabelAtCol(const uint32_t& _col){
        this->CheckDatasetValid();
        return this->labels[this->active_dataset].Col<T>(_row);
    }

};