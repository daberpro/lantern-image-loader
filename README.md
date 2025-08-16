# Lantern Image Loader

Lantern Image Loader is a C++ library designed for efficient and high-performance image loading and processing using multithreading. It automatically manages image loading, resizing, and caching in memory, making it an ideal solution for applications requiring high-speed image data pipelines, such as in machine learning or computer vision.

## Key Features

  - 🚀 **Thread-Based Loading**: Utilizes a producer-consumer queue to load images asynchronously on a separate thread. This allows the application to process already loaded images while the next ones are being prepared.
  - 🗂️ **Dataset Management**: Supports the creation and selection of multiple datasets. Each dataset can be populated with images from various folders.
  - 🖼️ **Automatic Image Resizing**: Automatically resizes loaded images to a specified width and height using the `stb_image_resize2` library.
  - 🎨 **Color and Grayscale Support**: Capable of loading either color (3-channel) or grayscale (1-channel) images, based on a template parameter.
  - 🏷️ **Automatic Labeling**: Automatically extracts the parent folder name of an image as its label, which is particularly useful for datasets organized by class directories.
  - 💾 **In-Memory Caching**: Images are cached in memory for fast access.
  - ⚙️ **ArrayFire Integration**: Provides methods to directly convert loaded images into `af::array` objects for further processing, such as pixel normalization for machine learning models.
  - 📊 **CSV Support**: Has the ability to load labels from a CSV file.

-----

## Requirements

  - `stb_image.h` and `stb_image_resize2.h` libraries.
  - ArrayFire library (optional, only required if using the `GetAsAF` method).
  - C++17 standard or newer.

-----

## How to Use

### 1\. Initializing the Loader

Initialize `LanternImageLoader` with template parameters:

  - `TOTAL_IMAGES`: The total number of images to be stored in the cache.
  - `IMG_WIDTH`: The desired image width.
  - `IMG_HEIGHT`: The desired image height.
  - `IsColor`: `true` to load color images (3 channels), `false` for grayscale (1 channel).

<!-- end list -->

```cpp
// Example: cache 100 images, resized to 224x224, in color
LanternImageLoader<100, 224, 224, true> imageLoader;
```

### 2\. Creating and Selecting a Dataset

You can create a new dataset and set it as the active one for modifications:

```cpp
imageLoader.CreateDatasetForFolder("my_dataset");
imageLoader.SelectDatasetToModify("my_dataset");
```

### 3\. Loading Images

Add images from class-organized directories. The loader will scan valid subdirectories and populate the dataset:

```cpp
imageLoader.GetImagesDataFromFolder("/path/to/my_dataset/cat");
imageLoader.GetImagesDataFromFolder("/path/to/my_dataset/dog");
```

### 4\. Example: Creating and Using Train/Test Datasets

To manage separate training and testing datasets, you can create a unique dataset for each and populate them with their respective image directories.

First, create the "train" dataset and add images to it.

```cpp
// Create and select the training dataset
imageLoader.CreateDatasetForFolder("train_dataset");
imageLoader.SelectDatasetToModify("train_dataset");

// Populate the training dataset with images from the train folder
imageLoader.GetImagesDataFromFolder("/path/to/train/cat");
imageLoader.GetImagesDataFromFolder("/path/to/train/dog");
```

Next, create the "test" dataset and add images to it.

```cpp
// Create and select the testing dataset
imageLoader.CreateDatasetForFolder("test_dataset");
imageLoader.SelectDatasetToModify("test_dataset");

// Populate the testing dataset with images from the test folder
imageLoader.GetImagesDataFromFolder("/path/to/test/cat");
imageLoader.GetImagesDataFromFolder("/path/to/test/dog");
```

To switch between datasets for loading, simply call `SelectDatasetToModify`. The `Run()` and `Get()` methods will then operate on the currently active dataset.

```cpp
// To load images for training:
imageLoader.SelectDatasetToModify("train_dataset");
imageLoader.Run();
// ... use Get() or GetAsAF() to retrieve training images ...

// To load images for testing, you would first stop the previous thread,
// select the new dataset, and then run again.
imageLoader.Stop();
imageLoader.SelectDatasetToModify("test_dataset");
imageLoader.Run();
// ... use Get() or GetAsAF() to retrieve testing images ...
```

### 5\. Running the Loader Thread

Call the `Run()` method to start the image loading thread:

```cpp
imageLoader.Run();
```

### 6\. Retrieving an Image

Use the `Get()` method to retrieve the next image from the queue. This method is blocking until an image is available.

```cpp
uint8_t* imageData = imageLoader.Get();
if (imageData != nullptr) {
    // Process image data...
}
```

### 7\. Retrieving an Image with Label (Folder-Based)

Use the `GetAsAF` method to retrieve an image converted to an `af::array` along with its label (the parent folder name).

```cpp
af::array image_array;
std::string label;
imageLoader.GetAsAF(image_array, label);

// 'image_array' now contains normalized image data,
// and 'label' contains its class name.
```

### 8\. Stopping the Loader

When you are finished, always call the `Stop()` method to safely terminate the loading thread and clean up resources:

```cpp
imageLoader.Stop();
```

-----

## Full Example
```cpp
#include <iostream>
#include "../pch.h"
#include "../headers/Loader.h"

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
```

## File Structure

  - `Loader.h`: The main header file containing the `LanternImageLoader` class definition.
  - `stb_image.h`: Third-party library for loading images.
  - `stb_image_resize2.h`: Third-party library for image resizing.
  - `Vector.h`: Utility library for `lantern::utility::Vector`.
  - `DataProcessing.h`: Utility library for `lantern::data::GetRandomSampleClassIndex`.
  - `File.h`: Utility library for `CSVFile` and `ReadCSVFile`.

-----
