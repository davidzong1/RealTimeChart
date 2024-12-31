#include "dzsem.hpp"

namespace dz_communicate
{
    void dzshm_queue::get_next_pos()
    {
        if (current_pos == queue_size - 1)
        {
            current_pos = 0;
        }
        else
        {
            current_pos++;
        }
    }
    void dzshm_queue::config(int array_size, bool H_or_L, int key)
    {
        this->H_or_L = H_or_L;
        shm_chain_ptr_cache.resize(array_size);
        std::string str_name;
        shm_ptr_cache.resize(array_size);
        queue_size = array_size;
        for (int i = 0; i < array_size; i++)
        {
            str_name = "DZ_shm_queue_" + std::to_string(key) + "_" + std::to_string(i);
            shm_name.push_back(str_name);
            if (H_or_L)
            {
                shm_ptr_cache[i] = new shared_memory_object(open_or_create, str_name.c_str(), read_write);
                shm_ptr_cache[i]->truncate(sizeof(share_zone)); // 初始化共享内存大小
            }
            else
            {
                shm_ptr_cache[i] = new shared_memory_object(open_only, str_name.c_str(), read_write);
            }
            regions_cache.push_back(mapped_region(*shm_ptr_cache[i], read_write));
            shm_chain_ptr_cache[i] = static_cast<share_zone *>(regions_cache[i].get_address());
            if (H_or_L)
            {
                shm_chain_ptr_cache[i]->has_write = false;
                shm_chain_ptr_cache[i]->has_read = true;
            }
        }
        config_flag = true;
    }
    /**
     * @brief 先摧毁已存在的共享内存再初始化
     * @param array_size 共享内存数组大小
     * @param H_or_L     高低位
     * @param key        共享内存key
     */
    void dzshm_queue::config_by_init_shm(int array_size, bool H_or_L, int key)
    {
        std::string str_cache;
        for (int i = 0; i < array_size; i++)
        {
            str_cache = "DZ_shm_queue_" + std::to_string(key) + "_" + std::to_string(i);
            shared_memory_object::remove(str_cache.c_str());
        }
        config(array_size, H_or_L, key);
    }
    bool dzshm_queue::push(char *data, int size)
    {
        share_zone *current_ptr = shm_chain_ptr_cache[current_pos];
        /* 前方未读 */
        if (!current_ptr->has_read)
        {
            return false;
        }
        memcpy(current_ptr->data, data, size);
        current_ptr->data_size = size;
        current_ptr->has_write = true;
        current_ptr->has_read = false;
        get_next_pos();
        return true;
    }
    bool dzshm_queue::pop(char **data, int &size)
    {
        share_zone *current_ptr = shm_chain_ptr_cache[current_pos];
        /* 后方未写 */
        if (!current_ptr->has_write)
        {
            return false;
        }
        size = current_ptr->data_size;
        memcpy(*data, current_ptr->data, current_ptr->data_size);
        current_ptr->has_write = false;
        current_ptr->has_read = true;
        get_next_pos();
        return true;
    }
    dzshm_queue::~dzshm_queue()
    {
        int size = shm_ptr_cache.size();
        if (!config_flag)
            return;
        if (H_or_L)
        {
            for (int i = 0; i < size; i++)
            {
                shared_memory_object::remove(shm_name[i].c_str());
            }
            shm_name.clear();
            regions_cache.clear();
            shm_ptr_cache.clear();
            shm_ptr_cache.shrink_to_fit();
            regions_cache.shrink_to_fit();
            shm_name.shrink_to_fit();
        }
        else
        {
            shm_name.clear();
            regions_cache.clear();
            shm_ptr_cache.clear();
            shm_ptr_cache.shrink_to_fit();
            regions_cache.shrink_to_fit();
            shm_name.shrink_to_fit();
        }
    }
}