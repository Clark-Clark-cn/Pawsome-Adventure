#include <httplib.h>

#include <mutex>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include "config.h"

Config* Config::instance;
Config* Config::defaultInstance;

std::mutex gMutex; // 全局互斥锁

std::string strText; // 文本内容

struct PlayerInfo {
    int progress = 0;
    int teamID = 0;
    int memberIndex = 0;
    int teamSize=1;
    int segmentLen=0;
    std::chrono::steady_clock::time_point lastHeartbeat;
};
std::unordered_map<int, PlayerInfo> players;
int nextPlayerID = 1;
const std::chrono::seconds timeout{ 10 };

bool TeamMode = false; // 是否启用组队模式（由配置与 /setMode 控制）
int nextTeamID = 1;
int TeamSize = 2;
int ServerPort = 8888;
int HeartbeatTimeoutSec = 10;

std::vector<std::string> gLines; // 预拆行结果

static std::vector<std::string> splitTextByLines(const std::string& text){
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while(std::getline(ss,line))lines.push_back(line);
    return lines;
}

static int sumLen(const std::vector<std::string>& lines,int l,int r){
    int sum=0;
    for(int i=l;i<r;i++)sum+=lines[i].size();
    return sum;
}

static std::vector<std::string> partitionLines(const std::vector<std::string>& lines, int parts) {
    std::vector<std::string> out(parts);
    int n = (int)lines.size();
    for(int i=0;i<parts;i++){
        int start=(int)n*i/parts;
        int end=(int)n*(i+1)/parts;
        std::ostringstream oss;
        for(int j=start;j<end;j++){
            oss << lines[j];
            if(j < end - 1) oss << '\n';
        }
        out[i] = oss.str();
    }
    return out;
}

static bool parseBool(const std::string& v, bool defaultValue = false) {
    if (v == "1" || v == "true" || v == "yes" || v == "on" || v == "team") return true;
    if (v == "0" || v == "false" || v == "no" || v == "off" || v == "solo") return false;
    return defaultValue;
}

int main(int argc, char **argv)
{
    // Initialize Config (loads defaults + file values)
    Config* cfg = Config::getInstance("server.ini");
    TeamMode = (bool)cfg->get("team_mode");
    TeamSize = (int)cfg->get("team_size");
    if (TeamSize < 1) TeamSize = 1;
    HeartbeatTimeoutSec = (int)cfg->get("heartbeat_timeout");
    if (HeartbeatTimeoutSec < 1) HeartbeatTimeoutSec = 10;
    ServerPort = (int)cfg->get("port");
    if (ServerPort <= 0) ServerPort = 8888;
    cfg->save("server.ini");

    std::string textFile = (std::string)cfg->get("text_file");
    std::ifstream file(textFile);
    if (!file.good())
    {
        std::cerr << "Failed to open text: " << textFile << std::endl;
        return -1;
    }
    std::stringstream strStream{};
    strStream << file.rdbuf();
    strText = strStream.str();
    file.close();
    gLines = splitTextByLines(strText);

    httplib::Server server;

    server.Post("/login", [&](const httplib::Request &req, httplib::Response &res){
        std::lock_guard<std::mutex> lock(gMutex);
		int id = nextPlayerID++;

        // 由服务端统一控制是否启用组队模式
        bool useTeam = TeamMode;
        PlayerInfo info{};
        info.progress = 0;
		info.lastHeartbeat = std::chrono::steady_clock::now();
        if (useTeam)
        {
            info.teamSize = TeamSize;
            info.teamID = ((id - 1) / info.teamSize + 1);
			info.memberIndex = (id - 1) % info.teamSize;
        }
        else
        {
            info.teamSize = 1;
			info.teamID = id;
			info.memberIndex = 0;
        }
        if(info.teamSize<=1){
            info.segmentLen = sumLen(gLines, 0, (int)gLines.size());
        }else{
            int n=gLines.size();
            int start=(n*info.memberIndex)/info.teamSize;
            int end=(n*(info.memberIndex+1))/info.teamSize;
            info.segmentLen = sumLen(gLines, start, end);
        }
		players[id] = info;

        std::ostringstream os;
		os << id << ',' << info.teamID << ',' << info.memberIndex << ',' << info.teamSize;
		res.set_content(os.str(), "text/plain");
        std::cout << "player: " << id << "join. mode=" << (useTeam ? "team" : "solo") <<
         " teamID=" << info.teamID << " memberIndex=" << info.memberIndex << " teamSize"
          << info.teamSize << " segmentLen=" << info.segmentLen << std::endl;
    });

    server.Post("/queryText", [&](const httplib::Request& req, httplib::Response& res)
        {
            std::lock_guard lock(gMutex);
			if (!req.has_param("id"))
			{
                res.set_content(strText, "text/plain");
				return;
			}
			int id = std::stoi(req.get_param_value("id"));
            auto it = players.find(id);
			if (it==players.end())
			{
                res.set_content(strText, "text/plain");
                return;
			}
			const PlayerInfo& info = it->second;
			if (info.teamSize<=1)
			{
                res.set_content(strText, "text/plain");
                return;
			}
            int n=gLines.size();
            int start=(n*info.memberIndex)/info.teamSize;
            int end=(n*(info.memberIndex+1))/info.teamSize;
            std::ostringstream oss;
            for(int j=start;j<end;j++){
                oss << gLines[j];
                if(j < end - 1) oss << '\n';
            }
            res.set_content(oss.str(), "text/plain");
        });

    server.Post("/sync", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(gMutex);
        auto idStr = req.get_param_value("id");
        auto progStr = req.get_param_value("progress");
        int id = std::stoi(idStr);
        int progress = std::stoi(progStr);
        auto now = std::chrono::steady_clock::now();

        auto& info = players[id];
        info.progress = progress;
		info.lastHeartbeat = now;

        const std::chrono::seconds timeout{ HeartbeatTimeoutSec };
        for (auto it = players.begin(); it != players.end();) {
            if (now - it->second.lastHeartbeat > timeout)it = players.erase(it);
            else ++it;
        }
        std::ostringstream payload;
        bool first = true;
        for (const auto& [pid, info] : players) {
            if (!first)payload << ';';
            payload << pid << ':' << info.progress << ':' << info.teamID<<':'<<info.segmentLen;
            first = false;
        }
        res.set_content(payload.str(), "text/plain");
        });

    server.Post("/exit", [&](const httplib::Request& req, httplib::Response& res)
        {
			std::lock_guard<std::mutex> lock(gMutex);
            int id = std::stoi(req.get_param_value("id"));
            players.erase(id);
            res.set_content("ok", "text/plain");
			std::cout << "player: " << id << "exit the game" << std::endl;
        });

    server.Get("/mode", [&](const httplib::Request& req, httplib::Response& res)
        {
            std::lock_guard lock(gMutex);
            std::ostringstream os;
            os << (TeamMode ? "team" : "solo") << "," << TeamSize;
            res.set_content(os.str(), "text/plain");
        });
    server.Post("/setMode", [&](const httplib::Request& req, httplib::Response& res)
        {
            std::lock_guard lock(gMutex);
            if (!req.has_param("mode"))
            {
                res.status = 400;
                res.set_content("missing mode", "text/plain");
                return;
            }
            bool newMode = parseBool(req.get_param_value("mode"), TeamMode);
            int newTeamSize = TeamSize;
            if (req.has_param("teamSize"))
            {
                newTeamSize = std::max(1, std::stoi(req.get_param_value("teamSize")));
            }
            if (newMode != TeamMode || newTeamSize != TeamSize)
            {
                players.clear();
                nextPlayerID = 1;
            }
            TeamMode = newMode;
            TeamSize = newTeamSize;
            // 写回配置：先从磁盘重载，避免覆盖用户的其它手工修改
            cfg->set("team_mode", ConfigItem(TeamMode));
            cfg->set("team_size", ConfigItem(TeamSize));
            cfg->save("server.ini");
            std::ostringstream os;
            os << (TeamMode ? "team" : "solo") << "," << TeamSize;
            res.set_content(os.str(), "text/plain");
        });
	std::cout << "Starting server on port " << ServerPort << "..." << std::endl;
    server.listen("0.0.0.0", ServerPort);
	std::cout << "Server has stopped." << std::endl;

    return 0;
}


