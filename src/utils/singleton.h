#include <mutex>
#include <atomic>

template <typename T>
class Singleton
{
public:
    static T& Instance() 
    {
        T* pTemp = m_pInstance.load(std::memory_order_acquire);
        if (pTemp == nullptr)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            pTemp = m_pInstance.load(std::memory_order_relaxed);
            if (pTemp == nullptr)
            {
                pTemp = new T();
                m_pInstance.store(pTemp, std::memory_order_release);
            }
        }
        return *pTemp;
    }

protected:
    Singleton(void)
    {
        //用于实例化，否则garbo_不会实例化
        auto buildCG __attribute__((unused)) = &garbo_;
    }

    virtual ~Singleton(void) { }

    class CGarbo
    {
    public:
        ~CGarbo()
        {
            if(Singleton::m_pInstance)
            {
                T* tmp = Singleton::m_pInstance.load(std::memory_order_acquire);
                m_pInstance.store(nullptr, std::memory_order_release);
                delete tmp;
            }
        }
    };

private:
    Singleton(const Singleton& rhs)  = delete;
    Singleton(const Singleton&& rhs)  = delete;
    Singleton& operator = (const Singleton& rhs) = delete;
    Singleton& operator = (const Singleton&& rhs) = delete;

    static CGarbo garbo_;
    static std::mutex m_mutex;
    static std::atomic<T*> volatile m_pInstance;
};

template <typename T>
std::atomic<T*> volatile Singleton<T>::m_pInstance{nullptr};

template <typename T>
std::mutex Singleton<T>::m_mutex{};

template <typename T>
typename Singleton<T>::CGarbo Singleton<T>::garbo_{};
