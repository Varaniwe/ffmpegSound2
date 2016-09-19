
#include "usefulFunctions.h"

std::wstring utf8_decode(const std::string &str)
{
    if (str.empty()) 
        return std::wstring();
    int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


std::shared_ptr<char> dup_wchar_to_utf8(wchar_t *w)
{
    //Encoding
    int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
    std::shared_ptr<char> s(new char[l]);
    if (s)
        WideCharToMultiByte(CP_UTF8, 0, w, -1, s.get(), l, 0, 0);
    return s;
}


bool enumerate_devices(std::vector<std::string> *deviceNames)
{
    ICreateDevEnum *pDevEnum = NULL;
    CoInitialize(pDevEnum);
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);
    if (hr != S_OK)
    {
        return false;
    }
    IEnumMoniker *pEnum;
    hr = pDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEnum, 0);
    if (hr != S_OK)
    {
        return false;
    }

    IMoniker *pMoniker = NULL;    

    const std::locale locale("");
    typedef std::codecvt<wchar_t, char, std::mbstate_t> converter_type;
    const converter_type& converter = std::use_facet<converter_type>(locale);

    while (S_OK == pEnum->Next(1, &pMoniker, NULL))
    {
        IPropertyBag *pPropBag;
        LPOLESTR str = 0;
        hr = pMoniker->GetDisplayName(0, 0, &str);
        if (SUCCEEDED(hr))
        {
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
            if (SUCCEEDED(hr))
            {
                VARIANT var;
                VariantInit(&var);                                

                hr = pPropBag->Read(L"FriendlyName", &var, 0);

                std::wstring temp_wstr = (std::wstring)(var.bstrVal);
                std::string fName = "";

                std::vector<char> to(temp_wstr.length() * converter.max_length());
                std::mbstate_t state;
                const wchar_t* from_next;
                char* to_next;
                const converter_type::result result = converter.out(state, temp_wstr.data(), temp_wstr.data() + temp_wstr.length(), from_next, &to[0], &to[0] + to.size(), to_next);
                if (result == converter_type::ok || result == converter_type::noconv)
                    fName = std::string(&to[0], to_next);

                deviceNames->push_back(fName);
            }
            else {
                std::cout << "Could not bind to storage\n" << std::endl;
            }
        }
    } 

    return (deviceNames->size() > 0);
}

int get_device_index(const std::vector<std::string>& device_names)
{
    int dev_number = 1;
    bool valid = false;

    while (!valid)
    {
        valid = true; //Assume the cin will be an integer.
        
        printf("Select input device:\n");
        dev_number = 1;
        for (std::string s : device_names)
        {
            printf("%d. %s\n", dev_number, s.c_str());
            ++dev_number;
        }
        printf("Device number: ");
        std::cin >> dev_number;
        if (std::cin.fail() || dev_number > device_names.size() || dev_number <= 0)
        {
            printf("Incorrect device number\n");
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            valid = false;
        }
    }
        
    return dev_number - 1;
}


char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

