#!/usr/bin/env python3
"""
Batch questionnaire generation for all students.
生成随机但合理的生活习惯问卷数据，提交到后端 API。
"""
import requests, random, json, sys

BASE = sys.argv[1] if len(sys.argv) > 1 else "http://localhost:8080"

# --- 1. Admin login ---
resp = requests.post(f"{BASE}/api/admin/login",
    json={"studentNo": "admin", "password": "admin123"})
admin_token = resp.json()["data"]["token"]
headers = {"Authorization": f"Bearer {admin_token}"}

# --- 2. Get all students ---
resp = requests.get(f"{BASE}/api/admin/users", headers=headers)
users = [u for u in resp.json()["data"]["users"] if u["role"] == "student"]
print(f"Found {len(users)} students")

# --- 3. Random pools for realistic data ---
MBTI_LIST = ["INTJ","INTP","ENTJ","ENTP","INFJ","INFP","ENFJ","ENFP",
             "ISTJ","ISFJ","ESTJ","ESFJ","ISTP","ISFP","ESTP","ESFP"]
SLEEP_OPTS   = ["early", "normal", "late"]
GAMING_OPTS  = ["never", "sometimes", "often"]
VETO_ALL     = ["smoke_alcohol","midnight_gaming","loud_speaker","opposite_sex",
                "bad_hygiene","over_demand","boundary_violation","pets"]
DESC_TEMPLATES = [
    "性格温和，喜欢安静的环境，作息规律，爱干净。",
    "开朗外向，喜欢社交和运动，周末偶尔熬夜打游戏。",
    "学霸型选手，经常泡图书馆，作息严格，宿舍只是睡觉的地方。",
    "宅男/宅女型，喜欢在宿舍看剧刷手机，比较随和，不太计较细节。",
    "音乐爱好者，偶尔会外放音乐但会控制音量，尊重室友。",
    "早睡早起型，注重个人卫生，喜欢宿舍整洁有序。",
]
EXPECT_TEMPLATES = [
    "希望室友不要深夜打游戏开麦，保持基本卫生即可。",
    "希望室友尊重彼此空间，不要随便翻别人的东西。",
    "希望室友作息不要太离谱，能保持公共区域清洁。",
    "希望室友性格随和，大家互相理解互相包容。",
]

# --- 4. Submit questionnaires ---
success, fail = 0, 0
for i, u in enumerate(users):
    uid = u["userId"]
    sno = u["studentNo"]
    gender = u.get("gender", random.choice(["male","female"]))
    college = u.get("college", "计算机学院")
    major = u.get("major", "软件工程")

    # Generate varied profile
    sleep = random.choice(SLEEP_OPTS)
    hygiene = random.choice([2,3,4,5])
    noise = random.choice([2,3,4,5])
    temp = random.choice([20,22,24,26,28])
    social = random.choice([1,2,3,4,5])
    gaming = random.choice(GAMING_OPTS)
    mbti = random.choice(MBTI_LIST)

    # 1-3 veto items (avg 2)
    n_veto = random.choices([1,2,3], weights=[40,40,20])[0]
    vetos = random.sample(VETO_ALL, n_veto)

    sim_w = round(random.uniform(0.3, 0.6), 2)
    comp_w = round(random.uniform(0.1, 0.3), 2)
    veto_w = round(1.0 - sim_w - comp_w, 2)

    payload = {
        "basicInfo": {
            "gender": gender,
            "college": college,
            "major": major
        },
        "questionnaire": {
            "sleepSchedule": sleep,
            "hygieneLevel": hygiene,
            "noiseTolerance": noise,
            "temperaturePreference": temp,
            "socialPreference": social,
            "gamingBehavior": gaming,
            "mbtiType": mbti
        },
        "preference": {
            "similarityWeight": sim_w,
            "complementarityWeight": comp_w,
            "vetoSafetyWeight": veto_w
        },
        "vetoItems": vetos,
        "openText": {
            "selfDescription": f"{sno}同学，{college}{major}，{random.choice(DESC_TEMPLATES)}",
            "roommateExpectation": random.choice(EXPECT_TEMPLATES)
        }
    }

    try:
        r = requests.post(
            f"{BASE}/api/student/questionnaire/submit",
            headers={"Content-Type": "application/json",
                     "Authorization": f"Bearer tk_test"},
            json=payload, timeout=10
        )
        if r.status_code == 200 and r.json().get("code") == 200:
            success += 1
        else:
            fail += 1
            if fail <= 3:
                print(f"  FAIL [{sno}]: {r.text[:120]}")
    except Exception as e:
        fail += 1
        if fail <= 3:
            print(f"  ERR [{sno}]: {e}")

    if (i+1) % 20 == 0:
        print(f"  ... {i+1}/{len(users)} done ({success} ok, {fail} fail)")

print(f"\n>>> Done: {success} success, {fail} failed out of {len(users)} students <<<")
