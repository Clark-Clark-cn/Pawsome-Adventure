#pragma once

#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>

class ConfigItem{
    std::string value;
public:
    ConfigItem(){}
    ConfigItem(const char* v) : value(v) {}
    ConfigItem(int v) : value(std::to_string(v)) {}
    ConfigItem(float v) : value(std::to_string(v)) {}
    ConfigItem(bool v) : value(v ? "true" : "false") {}
    ~ConfigItem(){}
    std::string getValue() const { return value; }
    void setValue(const std::string& val) { value = val; }

    std::string toSaveString(const std::string& key) const {return key + "=" + value;}
    int toInt() const { return std::stoi(value); }
    float toFloat() const { return std::stof(value); }
    bool toBool() const { return value == "true"; }

    operator std::string() const { return value; }
    operator int() const { return toInt(); }
    operator float() const { return toFloat(); }
    operator bool() const { return toBool(); }
};

class Config {
    std::unordered_map<std::string, ConfigItem> items;
    static Config* instance;
    static Config* defaultInstance;
    bool isDefaultConfig = false;
    std::string filePath;
    Config() : isDefaultConfig(true) {  }
    Config(const std::string& path, bool isDefault = false) : filePath(path), isDefaultConfig(isDefault) {
        if(isDefaultConfig)return;
        load(path);
        setDefault();
        printConfig();
    }
    ~Config() {}
    void load(const std::string& path);
    void setDefault();
public:
    static Config* getInstance(const std::string& path="") {
        if (!instance) {
            defaultInstance = new Config();
            instance = new Config(path);
        }
        return instance;
    }
    void reload(const std::string& path=""){
        if(!path.empty()) filePath = path;
        if(isDefaultConfig) return;
        items.clear();
        load(filePath);
        setDefault();
    }
    void save(const std::string& path){
        if(isDefaultConfig)return;
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << path << std::endl;
            return;
        }
        for (const auto& [key, item] : items) {
            file << item.toSaveString(key) << std::endl;
        }
        file.close();
    }
    void set(const std::string& key, const ConfigItem& item) {items[key] = item;}
    void setifno(const std::string& key, const ConfigItem& item) {
        defaultInstance->set(key,item);
        if(items.find(key) == items.end()) items[key] = item;
    }
    const ConfigItem& get(const std::string& key) const{
        auto it = items.find(key);
        if (it != items.end()) {
            return it->second;
        }
        if(!isDefaultConfig&&defaultInstance){
            return defaultInstance->get(key);
        }
        std::cout<<"Config key not found: "<<key<<std::endl;
        return ConfigItem(0);
    }
    void printConfig()const 
    {
	    for (const auto& c:items)
	    {
			std::cout << c.second.toSaveString(c.first) << std::endl;
	    }
    }

};

// Inline implementations to make the class usable from multiple TUs (client/server)
inline void Config::load(const std::string& path){
    std::ifstream file(path);
    if(!file.is_open()){
        std::cerr << "Failed to open config file: " << path << std::endl;
        return;
    }
    std::string line;
    while(std::getline(file, line)){
        // trim
        auto ltrim=[&](std::string &s){ s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){return !std::isspace(ch);})); };
        auto rtrim=[&](std::string &s){ s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){return !std::isspace(ch);}).base(), s.end()); };
        ltrim(line); rtrim(line);
        if(line.empty() || line[0]=='#' || line[0]==';') continue;
        auto pos = line.find('=');
        if(pos==std::string::npos){
            // allow plain server address file for backward compatibility
            items["server"] = ConfigItem(line.c_str());
            continue;
        }
        std::string key = line.substr(0,pos);
        std::string val = line.substr(pos+1);
        ltrim(key); rtrim(key); ltrim(val); rtrim(val);
        items[key] = ConfigItem(val.c_str());
    }
}

inline void Config::setDefault(){
    // Server defaults
    setifno("team_mode", ConfigItem(false));
    setifno("port", ConfigItem(8888));
    setifno("text_file", ConfigItem("text.txt"));
    setifno("heartbeat_timeout", ConfigItem(10));
    setifno("team_size", ConfigItem(2));
    // Client defaults
    setifno("server", ConfigItem("http://127.0.0.1:8888"));
}