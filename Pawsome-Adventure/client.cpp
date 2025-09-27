#include <httplib.h>

#include "path.h"
#include "player.h"
#include "config.h"

#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <codecvt>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

enum class Stage
{
    Waiting, // 等待玩家加入
    Ready,   // 准备起跑倒计时
    Racing   // 正在比赛中
};

int valCountdown = 4;         // 起跑倒计时
Stage stage = Stage::Waiting; // 当前游戏阶段

int idPlayer = 0; // 玩家序号
struct RemotePlayerState
{
    int id = 0;
    int progress = 0;
    bool alive = false;
    int teamID = 0;
    int memberIndex = 0;
    int segmentLen = 0;
    std::unique_ptr<Player> avatar;
};

struct TeamAggregate{
    int teamID = 0;
    int memberCount = 0;
    int totalProgress = 0;
    int totalSegment = 0;
    bool alive = false;
};

int myTeamID = 0;

std::mutex PlayersMutex;
std::unordered_map<int, RemotePlayerState> Players;
// 每个队伍仅渲染一个 Avatar
std::unordered_map<int, std::unique_ptr<Player>> TeamAvatars;
std::vector<int> drawTeams; // 本帧需要绘制的队伍ID
std::atomic_int myProgress = 0;
std::atomic_int focusID = -1;
std::atomic<bool> exited = false; // 是否退出游戏
int numTotalChar = 0;             // 全部字符数

Path path = Path(
    {{842, 842},
     {1322, 842},
     {1322, 422},
     {2762, 442},
     {2762, 842},
     {3162, 842},
     {3162, 1722},
     {2122, 1722},
     {2122, 1562},
     {842, 1562},
     {842, 842}}); // 角色移动路径对象

int idxLine = 0; // 当前文本行索引
int idxChar = 0; // 当前文本字符索引
int myMemberIndex = 0;
int myTeamSize = 2;
bool soloMode = true;
std::string strText;                  // 文本内容
std::vector<std::string> strLineList; // 文本行列表

Atlas atlas_1pIdleUp;    // 玩家1向上闲置动画图集
Atlas atlas_1pIdleDown;  // 玩家1向下闲置动画图集
Atlas atlas_1pIdleLeft;  // 玩家1向左闲置动画图集
Atlas atlas_1pIdleRight; // 玩家1向右闲置动画图集
Atlas atlas_1pRunUp;     // 玩家1向上奔跑动画图集
Atlas atlas_1pRunDown;   // 玩家1向下奔跑动画图集
Atlas atlas_1pRunLeft;   // 玩家1向左奔跑动画图集
Atlas atlas_1pRunRight;  // 玩家1向右奔跑动画图集
Atlas atlas_2pIdleUp;    // 玩家2向上闲置动画图集
Atlas atlas_2pIdleDown;  // 玩家2向下闲置动画图集
Atlas atlas_2pIdleLeft;  // 玩家2向左闲置动画图集
Atlas atlas_2pIdleRight; // 玩家2向右闲置动画图集
Atlas atlas_2pRunUp;     // 玩家2向上奔跑动画图集
Atlas atlas_2pRunDown;   // 玩家2向下奔跑动画图集
Atlas atlas_2pRunLeft;   // 玩家2向左奔跑动画图集
Atlas atlas_2pRunRight;  // 玩家2向右奔跑动画图集

IMAGE imgUi_1;       // 界面文本1
IMAGE imgUi_2;       // 界面文本2
IMAGE imgUi_3;       // 界面文本3
IMAGE imgUiFight;    // 界面文本FIGHT
IMAGE imgUiTextbox;  // 界面文本框
IMAGE imgBackground; // 背景图

std::mutex web_mutex;
std::string strAddress;            // 服务器地址
httplib::Client *client = nullptr; // HTTP客户端对象

std::vector<RemotePlayerState *> drawList;
Timer timerCountdown;
WNDPROC prevWndProc = nullptr;
// Define config singletons for this TU
Config* Config::instance;
Config* Config::defaultInstance;

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
    {
        std::lock_guard<std::mutex> lock(web_mutex);
        if (client)
            client->Post("/exit", httplib::Params{{"id", std::to_string(idPlayer)}});
        exited = true;
        PostQuitMessage(0);
    }
        return 0;
    }

    return CallWindowProc(prevWndProc, hwnd, uMsg, wParam, lParam);
}

int pickNextFocus(int dir)
{
    std::lock_guard lock(PlayersMutex);
    if (Players.empty())
        return -1;
    std::vector<int> ids;
    ids.reserve(Players.size());
    for (auto &p : Players)
        if (p.second.alive && p.second.avatar)
            ids.push_back(p.first);

    if (ids.empty())
        return -1;
    std::sort(ids.begin(), ids.end());
    int cur = focusID;
    auto it = std::find(ids.begin(), ids.end(), cur);
    int idx = (it == ids.end()) ? 0 : int(it - ids.begin());

    idx = (idx + dir + (int)ids.size()) % (int)ids.size();
    return ids[idx];
}

void loadResources(HWND hwnd)
{
    AddFontResourceEx(L"resources/IPix.ttf", FR_PRIVATE, NULL);

    atlas_1pIdleUp.load(L"resources/hajimi_idle_back_%d.png", 4);
    atlas_1pIdleDown.load(L"resources/hajimi_idle_front_%d.png", 4);
    atlas_1pIdleLeft.load(L"resources/hajimi_idle_left_%d.png", 4);
    atlas_1pIdleRight.load(L"resources/hajimi_idle_right_%d.png", 4);
    atlas_1pRunUp.load(L"resources/hajimi_run_back_%d.png", 4);
    atlas_1pRunDown.load(L"resources/hajimi_run_front_%d.png", 4);
    atlas_1pRunLeft.load(L"resources/hajimi_run_left_%d.png", 4);
    atlas_1pRunRight.load(L"resources/hajimi_run_right_%d.png", 4);

    atlas_2pIdleUp.load(L"resources/manbo_idle_back_%d.png", 4);
    atlas_2pIdleDown.load(L"resources/manbo_idle_front_%d.png", 4);
    atlas_2pIdleLeft.load(L"resources/manbo_idle_left_%d.png", 4);
    atlas_2pIdleRight.load(L"resources/manbo_idle_right_%d.png", 4);
    atlas_2pRunUp.load(L"resources/manbo_run_back_%d.png", 4);
    atlas_2pRunDown.load(L"resources/manbo_run_front_%d.png", 4);
    atlas_2pRunLeft.load(L"resources/manbo_run_left_%d.png", 4);
    atlas_2pRunRight.load(L"resources/manbo_run_right_%d.png", 4);

    loadimage(&imgUi_1, L"resources/ui_1.png");
    loadimage(&imgUi_2, L"resources/ui_2.png");
    loadimage(&imgUi_3, L"resources/ui_3.png");
    loadimage(&imgUiFight, L"resources/ui_fight.png");
    loadimage(&imgUiTextbox, L"resources/ui_textbox.png");
    loadimage(&imgBackground, L"resources/background.png");

    loadAudio(L"resources/bgm.mp3", L"bgm");
    loadAudio(L"resources/1p_win.mp3", L"1p_win");
    loadAudio(L"resources/2p_win.mp3", L"2p_win");
    loadAudio(L"resources/click_1.mp3", L"click_1");
    loadAudio(L"resources/click_2.mp3", L"click_2");
    loadAudio(L"resources/click_3.mp3", L"click_3");
    loadAudio(L"resources/click_4.mp3", L"click_4");
    loadAudio(L"resources/ui_1.mp3", L"ui_1");
    loadAudio(L"resources/ui_2.mp3", L"ui_2");
    loadAudio(L"resources/ui_3.mp3", L"ui_3");
    loadAudio(L"resources/ui_fight.mp3", L"ui_fight");

    Config* cfg = Config::getInstance("client.ini");
    cfg->save("client.ini");
    strAddress = cfg->get("server");
}

void loginToServer(HWND hwnd)
{
    client = new httplib::Client(strAddress);
    client->set_keep_alive(true);

    httplib::Result result = client->Post("/login");
    if (!result || result->status != 200)
    {
        MessageBox(hwnd, L"unable to connect to server", L"login failed", MB_OK | MB_ICONERROR);
        std::cout << result.error() << std::endl;
        exit(-1);
    }

    std::stringstream ss(result->body);
    std::string tok;
    std::getline(ss, tok, ',');
    idPlayer = std::stoi(tok);
    std::getline(ss, tok, ',');
    myTeamID = std::stoi(tok);
    std::getline(ss, tok, ',');
    myMemberIndex = std::stoi(tok);
    std::getline(ss, tok, ',');
    myTeamSize = std::stoi(tok);

    if (idPlayer <= 0)
    {
        MessageBox(hwnd, L"server full", L"login failed", MB_OK | MB_ICONERROR);
        exit(-1);
    }
    focusID = myTeamID;

    strText = client->Post("/queryText", httplib::Params{{"id", std::to_string(idPlayer)}})->body;

    std::stringstream strStream(strText);
    std::string strLine;
    while (std::getline(strStream, strLine))
    {
        strLineList.push_back(strLine);
        numTotalChar += (int)strLine.length();
    }

    std::thread([&]()
                {
        using namespace std::chrono_literals;
        while (!exited)
        {
            httplib::Params parmas{
                {"id",std::to_string(idPlayer)},
                {"progress",std::to_string(myProgress)}
            };
            auto res = client->Post("/sync", parmas);
            if (res && res->status == 200) {
                std::lock_guard lock(PlayersMutex);
                std::unordered_set<int> seen;
                std::stringstream ss(res->body);
                std::string token;
                while (std::getline(ss, token, ';')) {
					auto d1 = token.find(':');
                    if (d1==std::string::npos)continue;
					auto d2 = token.find(':', d1 + 1);
                    auto d3 = (d2==std::string::npos)?std::string::npos:token.find(':', d2 + 1);

                    int pid = std::stoi(token.substr(0, d1));
                    int prog = std::stoi(token.substr(d1 + 1, (d2==std::string::npos)?std::string::npos:(d2 - d1 - 1)));
                    int tID = 0;
                    int seg = 0;
                    if(d2!=std::string::npos)
                        tID = std::stoi(token.substr(d2 + 1, (d3==std::string::npos)?std::string::npos:(d3 - d2 - 1)));
                    if(d3!=std::string::npos)
                        seg=std::stoi(token.substr(d3 + 1));

					seen.insert(pid);
                    auto& state = Players[pid];
                    if (!state.avatar) {
                        if (pid % 2 == 0)
                            state.avatar = std::make_unique<Player>(&atlas_1pIdleUp, &atlas_1pIdleDown, &atlas_1pIdleLeft, &atlas_1pIdleRight,
                                &atlas_1pRunUp, &atlas_1pRunDown, &atlas_1pRunLeft, &atlas_1pRunRight);
                        else state.avatar = std::make_unique<Player>(&atlas_2pIdleUp, &atlas_2pIdleDown, &atlas_2pIdleLeft, &atlas_2pIdleRight,
                            &atlas_2pRunUp, &atlas_2pRunDown, &atlas_2pRunLeft, &atlas_2pRunRight);
                        float d=(seg>0)?seg:(soloMode?numTotalChar:1.0f);
                        float t=std::clamp(prog/d, 0.0f, 1.0f);
                        state.avatar->setPosition(path.getPostitionAtProgress(t));
                        state.avatar->setTarget(state.avatar->getPosition());
                    }
                    state.id = pid;
                    state.progress = prog;
                    state.alive = true;
                    if(tID != 0)state.teamID = tID;
                    if(seg != 0)state.segmentLen = seg;
                    if(seg==0 && soloMode)state.segmentLen = numTotalChar;
                }
                for(auto& kv:Players){
                    kv.second.alive = seen.count(kv.first) > 0;
                }
            }
            std::this_thread::sleep_for(100ms);
        }
    }).detach();
}

void input()
{
    ExMessage msg;
    {
        std::lock_guard<std::mutex> lock(PlayersMutex);

        // 若当前对焦队伍不存在或不活跃，则回退到自身队伍或最小teamID
        std::unordered_set<int> aliveTeams;
        for (auto &kv : Players) {
            const auto &st = kv.second;
            if (st.alive) aliveTeams.insert(st.teamID ? st.teamID : kv.first);
        }
        if (!aliveTeams.empty() && aliveTeams.count(focusID) == 0) {
            int fb = -1;
            if (aliveTeams.count(myTeamID)) fb = myTeamID;
            else {
                fb = INT_MAX;
                for (int tid : aliveTeams) fb = std::min(fb, tid);
                if (fb == INT_MAX) fb = -1;
            }
            if (fb != -1) focusID = fb;
        }
    }
    MSG winmsg;
    while (PeekMessage(&winmsg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&winmsg);
        DispatchMessage(&winmsg);
    }
    //////////////////////////////////////// 处理玩家输入/////////////////////////////////////
    while (peekmessage(&msg))
    {
        if (stage != Stage::Racing)
            continue;
        if (msg.message == WM_MOUSEWHEEL)
        {
            int dir = (msg.wheel > 0) ? 1 : -1;
            // 在活跃队伍之间切换
            std::vector<int> aliveTeamIds;
            {
                std::lock_guard<std::mutex> lock(PlayersMutex);
                std::unordered_set<int> s;
                for (auto &kv : Players)
                    if (kv.second.alive) s.insert(kv.second.teamID ? kv.second.teamID : kv.first);
                aliveTeamIds.assign(s.begin(), s.end());
            }
            if (!aliveTeamIds.empty()) {
                std::sort(aliveTeamIds.begin(), aliveTeamIds.end());
                auto it = std::find(aliveTeamIds.begin(), aliveTeamIds.end(), focusID);
                int idx = (it == aliveTeamIds.end()) ? 0 : int(it - aliveTeamIds.begin());
                idx = (idx + dir + (int)aliveTeamIds.size()) % (int)aliveTeamIds.size();
                focusID = aliveTeamIds[idx];
            }
            continue;
        }
        if (msg.message == WM_RBUTTONDOWN)
        {
            // 回到自己队伍
            focusID = myTeamID;
            continue;
        }
        if (msg.message == WM_CHAR && idxLine < strLineList.size())
        {
            const std::string &strLine = strLineList[idxLine];
            if (strLine[idxChar] == msg.ch)
            {
                playAudio((L"click_" + std::to_wstring(rand() % 4 + 1)).c_str());
                ++myProgress;
                if (auto it = Players.find(idPlayer); it != Players.end())
                    it->second.progress = myProgress;

                idxChar++;
                if (idxChar >= strLine.length())
                {
                    idxChar = 0;
                    idxLine++;
                }
            }
        }
    }
}

void update(float delta)
{
    int alivePlayers = 0;
    int aliveTeams = 0;
    {
        std::lock_guard lock(PlayersMutex);
        std::unordered_set<int> teamSet;
        for (auto &[pid, state] : Players)
        {
            if (!state.alive) continue;
            alivePlayers++;
            int tid = state.teamID ? state.teamID : pid;
            teamSet.insert(tid);
        }
        aliveTeams = (int)teamSet.size();
    }
    bool readyToStart = false;
    if (soloMode)
    {
        // Solo 模式：至少 2 名玩家
        readyToStart = (alivePlayers >= 2);
    }
    else
    {
        // Team 模式：至少 2 个队伍
        readyToStart = (aliveTeams >= 2);
    }
    if (stage == Stage::Waiting && readyToStart)
        stage = Stage::Ready;
    if (stage == Stage::Ready)
        timerCountdown.onUpdate(delta);
    if (stage == Stage::Racing)
    {
        int winnerTeam = -1;
        {
            std::lock_guard lock(PlayersMutex);
            std::unordered_map<int, bool> teamAllDone;
            for (auto &[pid, state] : Players)
            {
                if (!state.alive)
                    continue;
                if (!teamAllDone.count(state.teamID))
                    teamAllDone[state.teamID] = true;
                if (state.segmentLen > 0 && state.progress < state.segmentLen)
                {
                    teamAllDone[state.teamID] = false;
                }
            }
            for (const auto &[tid, done] : teamAllDone)
            {
                if (done)
                {
                    winnerTeam = tid;
                    break;
                }
            }
            if (auto it = Players.find(idPlayer); it != Players.end() && it->second.teamID != 0)
            {
                myTeamID = it->second.teamID;
            }
        }
        if (winnerTeam != -1)
        {
            stopAudio(L"bgm");
            bool myTeamWin = (winnerTeam == myTeamID);
            playAudio(myTeamWin ? L"1p_win" : L"2p_win");
            MessageBox(NULL, myTeamWin ? L"Win!!!" : L"Lose!!!",
                       L"Game Over", MB_OK | MB_ICONINFORMATION);
            client->Post("/exit", httplib::Params{{"id", std::to_string(idPlayer)}});
            exited = true;
            exit(0);
        }
        // 聚合到队伍，队伍共用一个 Avatar
        std::unordered_map<int, TeamAggregate> agg;
        {
            std::lock_guard lock(PlayersMutex);
            for (auto &[pid, st] : Players) {
                if (!st.alive || !st.avatar) continue;
                int tid = st.teamID ? st.teamID : pid;
                auto &a = agg[tid];
                a.teamID = tid;
                a.memberCount++;
                a.alive = true;
                a.totalProgress += st.progress;
                a.totalSegment += (st.segmentLen > 0 ? st.segmentLen : (soloMode ? numTotalChar : 0));
            }
        }

        drawTeams.clear();
        for (auto &[tid, a] : agg) {
            drawTeams.push_back(tid);
            if (!TeamAvatars.count(tid)) {
                if (tid % 2 == 0)
                    TeamAvatars[tid] = std::make_unique<Player>(&atlas_1pIdleUp, &atlas_1pIdleDown, &atlas_1pIdleLeft, &atlas_1pIdleRight,
                        &atlas_1pRunUp, &atlas_1pRunDown, &atlas_1pRunLeft, &atlas_1pRunRight);
                else
                    TeamAvatars[tid] = std::make_unique<Player>(&atlas_2pIdleUp, &atlas_2pIdleDown, &atlas_2pIdleLeft, &atlas_2pIdleRight,
                        &atlas_2pRunUp, &atlas_2pRunDown, &atlas_2pRunLeft, &atlas_2pRunRight);
                float denom = (a.totalSegment > 0) ? (float)a.totalSegment : (soloMode ? (float)(a.memberCount * numTotalChar) : 1.0f);
                float t = std::clamp((float)a.totalProgress / denom, 0.0f, 1.0f);
                Vector2 pos = path.getPostitionAtProgress(t);
                TeamAvatars[tid]->setPosition(pos);
                TeamAvatars[tid]->setTarget(pos);
            } else {
                float denom = (a.totalSegment > 0) ? (float)a.totalSegment : (soloMode ? (float)(a.memberCount * numTotalChar) : 1.0f);
                float t = std::clamp((float)a.totalProgress / denom, 0.0f, 1.0f);
                Vector2 pos = path.getPostitionAtProgress(t);
                TeamAvatars[tid]->setTarget(pos);
            }
        }

        for (int tid : drawTeams) {
            TeamAvatars[tid]->onUpdate(delta);
        }

        std::sort(drawTeams.begin(), drawTeams.end(), [](int a, int b){ return a < b; });
    }
}

void render(Camera &cameraUi, Camera &cameraScene)
{

    /////////////////////////////////////// 处理画面绘制/////////////////////////////////////
    setbkcolor(RGB(0, 0, 0));
    cleardevice();

    if (stage == Stage::Waiting)
    {
        settextcolor(RGB(195, 195, 195));
        outtextxy(15, 675, L"competition is about to begin, waiting for other players...");
    }
    else
    {
        // 绘制背景图
        static const Rect rectBg = {
            .x = 0, .y = 0, .w = imgBackground.getwidth(), .h = imgBackground.getheight()};
        putimageEx(cameraScene, &imgBackground, &rectBg);

        // 绘制各队伍 Avatar
        for (int tid : drawTeams) {
            if (TeamAvatars.count(tid)) TeamAvatars[tid]->onRender(cameraScene);
        }
        // 摄像机对焦队伍
        int fid = focusID;
        auto chooseFallbackTeam = [&]() -> int {
            if (TeamAvatars.count(myTeamID)) return myTeamID;
            if (!drawTeams.empty()) return *std::min_element(drawTeams.begin(), drawTeams.end());
            return -1;
        };
        if (!TeamAvatars.count(fid)) {
            int fb = chooseFallbackTeam();
            if (fb != -1) {
                focusID = fb;
                fid = fb;
            }
        }
        if (TeamAvatars.count(fid))
            cameraScene.lookAt(TeamAvatars[fid]->getPosition());

        // 绘制倒计时
        switch (valCountdown)
        {
        case 3:
        {
            static const Rect rectUi_3 = {
                .x = 1280 / 2 - imgUi_3.getwidth() / 2, .y = 720 / 2 - imgUi_3.getheight() / 2, .w = imgUi_3.getwidth(), .h = imgUi_3.getheight()};
            putimageEx(cameraUi, &imgUi_3, &rectUi_3);
        }
        break;
        case 2:
        {
            static const Rect rectUi_2 =
                {
                    .x = 1280 / 2 - imgUi_2.getwidth() / 2, .y = 720 / 2 - imgUi_2.getheight() / 2, .w = imgUi_2.getwidth(), .h = imgUi_2.getheight()};
            putimageEx(cameraUi, &imgUi_2, &rectUi_2);
        }
        break;
        case 1:
        {
            static const Rect rectUi_1 =
                {
                    .x = 1280 / 2 - imgUi_1.getwidth() / 2, .y = 720 / 2 - imgUi_1.getheight() / 2, .w = imgUi_1.getwidth(), .h = imgUi_1.getheight()};
            putimageEx(cameraUi, &imgUi_1, &rectUi_1);
        }
        break;
        case 0:
        {
            static const Rect rectUi_fight =
                {
                    .x = 1280 / 2 - imgUiFight.getwidth() / 2, .y = 720 / 2 - imgUiFight.getheight() / 2, .w = imgUiFight.getwidth(), .h = imgUiFight.getheight()};
            putimageEx(cameraUi, &imgUiFight, &rectUi_fight);
        }
        break;
        default:
            break;
        }

        // 绘制界面
        if ((stage == Stage::Racing))
        {
            static const Rect rectTextbox =
                {
                    .x = 0,
                    .y = 720 - imgUiTextbox.getheight(),
                    .w = imgUiTextbox.getwidth(),
                    .h = imgUiTextbox.getheight()};
            if (idxLine < strLineList.size())
            {
                static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
                std::wstring wstrLine = convert.from_bytes(strLineList[idxLine]);
                std::wstring wstrCompleted = convert.from_bytes(strLineList[idxLine].substr(0, idxChar));
                putimageEx(cameraUi, &imgUiTextbox, &rectTextbox);
                settextcolor(RGB(125, 125, 125));
                outtextxy(185 + 2, rectTextbox.y + 65 + 2, wstrLine.c_str());
                settextcolor(RGB(25, 25, 25));
                outtextxy(185, rectTextbox.y + 65, wstrCompleted.c_str());
                settextcolor(RGB(0, 149, 217));
                outtextxy(185, rectTextbox.y + 65, wstrCompleted.c_str());
            }
            else
            {
                settextcolor(RGB(120, 200, 120));
                outtextxy(185, rectTextbox.y + 65, L"Section completed!");
            }
        }
    }
    FlushBatchDraw();
}

int main(int argc, char **argv)
{
    MessageBox(NULL,
               L"Start conditions:\n- Solo: at least 2 players\n- Team: at least 2 teams\n\nUse the mouse wheel to switch the focus team.\n"
               L"Right-click to return to your own team.\nType on the keyboard to move your player forward.",
               L"Tips", MB_ICONINFORMATION | MB_OK);
    //////////////////////////////////////处理数据初始化 //////////////////////////////////////
    using namespace std::chrono;

    HWND hwnd = initgraph(1280, 720 /*,EW_SHOWCONSOLE*/);
    prevWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
    SetWindowText(hwnd, L"Pawsome Adventure");
    settextstyle(28, 0, L"Ipix");

    setbkmode(TRANSPARENT);

    loadResources(hwnd);

    // 启动时先向服务端获取当前模式（使用 config.cfg 中的地址）
    {
        httplib::Client probe(strAddress);
        auto modeRes = probe.Get("/mode");
        if (modeRes && modeRes->status == 200) {
            // 形如: "team,2" 或 "solo,2"
            std::stringstream ms(modeRes->body);
            std::string m,t;
            std::getline(ms, m, ',');
            soloMode = (m != "team");
        }
    }

    loginToServer(hwnd);

    Camera cameraUi, cameraScene;
    cameraUi.setSize({1280, 720});
    cameraScene.setSize({1280, 720});

    timerCountdown.setOneShot(false);
    timerCountdown.setWaitTime(1.0f);
    timerCountdown.setOnTimeout([&]()
                                {
        valCountdown--;
       switch(valCountdown)
        {
            case 3: playAudio(L"ui_3"); break;
            case 2: playAudio(L"ui_2"); break;
            case 1: playAudio(L"ui_1"); break;
            case 0: playAudio(L"ui_fight");  break;
            case -1: 
                stage=Stage::Racing; 
              
                playAudio(L"bgm",true);
                break;
        } });

    constexpr nanoseconds frameDuration = nanoseconds(1000000000 / 144);
    steady_clock::time_point lastTick = steady_clock::now();

    BeginBatchDraw();

    while (!exited)
    {
        steady_clock::time_point frameStart = steady_clock::now();
        duration<float> delta = duration<float>(frameStart - lastTick);
        input();
        update(delta.count());
        render(cameraUi, cameraScene);

        lastTick = frameStart;
        nanoseconds sleepDuration = frameDuration - (steady_clock::now() - frameStart);
        if (sleepDuration > nanoseconds(0))
            std::this_thread::sleep_for(sleepDuration);
    }
    EndBatchDraw();
    MessageBox(hwnd, L"You have finished typing or the other player has left the game", L"Game Over", MB_OK | MB_ICONINFORMATION);

    return 0;
}