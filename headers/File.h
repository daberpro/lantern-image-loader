#pragma once
#include "../pch.h"
#include "Vector.h"

/**
 * @brief Lantern CSV file wrapper
 * @ingroup LanternFile
 */
class CSVFile
{
private:
    lantern::utility::Vector<lantern::utility::Vector<std::string>> data;

    /**
     * @brief Convert string into T type
     * @tparam T
     * @param _str
     * @return T
     */
    template <typename T>
    T ConvertFromString(const std::string &_str)
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            T value{};
            auto [ptr, e] = std::from_chars(_str.data(), _str.data() + _str.size(), value);
            if (e != std::errc{})
            {
                throw std::runtime_error("Error CSVFile, ivalid conversion");
            }
            return value;
        }
        else
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                return _str;
            }
            else
            {
                T value{};
                std::istringstream iss(_str);
                iss >> value;
                if (iss.fail() || !iss.eof())
                {
                    throw std::runtime_error("Error CSVFile,conversion failed or extra characters found.");
                }
                return value;
            }
        }
    }

public:
    CSVFile() {}
    CSVFile(CSVFile &&_file) noexcept
    {
        this->data.movePtrData(_file.data);
    }

    void operator=(CSVFile &&_file) noexcept
    {
        this->data.movePtrData(_file.data);
    }
    /**
     * @brief Get pointer to data inside CSV file
     * @return lantern::utility::Vector<lantern::utility::Vector<std::string>>
     */
    auto *GetDataPtr()
    {
        return &this->data;
    }

    /**
     * @brief Get data in specific row, col and cast to type T
     * @tparam T
     * @param row
     * @param col
     * @return T
     */
    template <typename T>
    T Get(const uint32_t &row, const uint32_t col)
    {
        if (col >= this->data.front().size())
        {
            throw std::runtime_error(std::format("Error CSVFile, cannot access column index \"{}\" out of bound", col));
        }
        if (row >= this->data.size())
        {
            throw std::runtime_error(std::format("Error CSVFile, cannot access row index \"{}\" out of bound", row));
        }
        return this->ConvertFromString<T>(this->data[row][col]);
    }

    /**
     * @brief Get column at index and cast to T type
     * @tparam T
     * @param _index
     * @return lantern::utility::Vector<T>
     */
    template <typename T>
    auto Col(const uint32_t &_index)
    {

        if (_index >= this->data.front().size())
        {
            throw std::runtime_error(std::format("Error CSVFile, cannot access column index \"{}\" out of bound", _index));
        }

        lantern::utility::Vector<T> result(this->data.size());
        result.explicitTotalItem(this->data.size());

        std::transform(
            this->data.begin(),
            this->data.end(),
            result.begin(),
            [&](const lantern::utility::Vector<std::string> &row) -> T
            {
                return this->ConvertFromString<T>(row[_index]);
            });
        return result;
    }

    /**
     * @brief Get row at index and cast to T type
     * @tparam T
     * @param _index
     * @return lantern::utility::Vector<T>
     */
    template <typename T>
    auto Row(const uint32_t &_index)
    {

        if (_index >= this->data.size())
        {
            throw std::runtime_error(std::format("Error CSVFile, cannot access row index \"{}\" out of bound", _index));
        }

        auto &data_ = this->data[_index];
        uint32_t allocated_size = data_.size();
        lantern::utility::Vector<T> result(allocated_size);
        result.explicitTotalItem(allocated_size);
        std::transform(
            data_.begin(),
            data_.end(),
            result.begin(),
            [&](const std::string_view &_str) -> T
            {
                return this->ConvertFromString<T>(_str);
            });
        return result;
    }

    auto GetPtrRow(const uint32_t &_index)
    {
        if (_index >= this->data.size())
        {
            throw std::runtime_error(std::format("Error CSVFile, cannot access row index \"{}\" out of bound", _index));
        }
        return &this->data[_index];
    }
};

/**
 * @brief Read CSV file to given path
 * @param _path
 * @return lantern::file::CSVFile
 * @ingroup LanternFile
 */
[[nodiscard]]
inline CSVFile ReadCSVFile(const std::filesystem::path &_path)
{

    CSVFile result;
    auto &data = (*result.GetDataPtr());

    if (!std::filesystem::exists(_path))
    {
        throw std::runtime_error(std::format("Error CSVReader, cannot access file path \"{}\" looks like deleted or moved", _path.string()));
    }

    // first check extension
    if (std::filesystem::is_regular_file(_path))
    {
        std::string ext = _path.extension().string(), col_data, line;
        std::transform(ext.begin(), ext.end(), ext.begin(), [](const char &d)
                       { return std::tolower(d); });
        if (ext.compare(".csv") == 0)
        {
            std::ifstream file(_path);
            if (!file.is_open())
            {
                throw std::runtime_error(std::format("Error CSVReader, failed to open file \"{}\"", ext));
            }

            while (std::getline(file, line))
            {
                data.push_back(lantern::utility::Vector<std::string>(20));
                std::stringstream ss(line);

                while (std::getline(ss, col_data, ','))
                {
                    data.back().push_back(col_data);
                }

                if (!data.empty())
                {
                    if (data.front().size() != data.back().size())
                    {
                        throw std::runtime_error(std::format("Error ReadCSVFile, the file \"{}\" has different columns sizes", _path.string()));
                    }
                }
            }
        }
        else
        {
            throw std::runtime_error(std::format("Error CSVReader, the file extension \"{}\" is not json file", ext));
        }
    }
    else
    {
        throw std::runtime_error(std::format("Error CSVReader, the path \"{}\" is not file path", _path.string()));
    }

    return result;
}
/**
 * @brief Read json file from given path
 * @param _path
 * @return nlohmann::json
 * @ingroup LanternFile
 */