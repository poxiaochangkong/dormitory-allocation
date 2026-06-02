#!/usr/bin/env python3
"""
直接生成 SQL 批量插入问卷数据。这是最快、最可靠的方式。
"""
import random, hashlib, time

random.seed(42)

MBTI = ["INTJ","INTP","ENTJ","ENTP","INFJ","INFP","ENFJ","ENFP",
        "ISTJ","ISFJ","ESTJ","ESFJ","ISTP","ISFP","ESTP","ESFP"]
SLEEP = ["early", "normal", "late"]
GAMING = ["never", "sometimes", "often"]
VETO_ALL = ["smoke_alcohol","midnight_gaming","loud_speaker","opposite_sex",
            "bad_hygiene","over_demand","boundary_violation","pets"]

def gen_id(prefix):
    return f"{prefix}{int(time.time()*1000)}_{random.randint(1000,9999)}"

def esc(s):
    return s.replace("'", "''")

# Student data: (user_id, student_no)
students = []
# We'll generate for studentNo 2024001 to 2024100
# Using the pattern from batch_import.sh: u_<timestamp>_<random>
# But we need actual user_ids from the DB
# Let's just generate SQL that assumes userId = f"u_{sno}"

sql_questionnaire = []
sql_preference = []
sql_veto = []
sql_open_text = []

ts = int(time.time() * 1000)

# Student IDs from batch import: u_1780386783398_5636, etc.
# We need to get them from DB. But to avoid dependency, let's
# just set a flag that the script reads from DB

import subprocess

result = subprocess.run(
    ["docker", "exec", "mysql-dorm", "mysql", "-u", "root", "-proot", "dorm_alloc",
     "-N", "-e", "SELECT user_id, student_no, gender, college, major FROM `user` WHERE role='student' ORDER BY student_no"],
    capture_output=True, text=True
)

lines = result.stdout.strip().split('\n')
students = []
for line in lines:
    parts = line.split('\t')
    if len(parts) >= 5:
        students.append({
            "uid": parts[0],
            "sno": parts[1],
            "gender": parts[2],
            "college": parts[3],
            "major": parts[4]
        })

print(f"-- Processing {len(students)} students")

for s in students:
    uid = s["uid"]
    sno = s["sno"]
    gender = s["gender"]
    college = s["college"]
    major = s["major"]

    sleep = random.choice(SLEEP)
    hygiene = random.choice([2,3,4,5])
    noise = random.choice([2,3,4,5])
    temp = random.choice([20,22,24,26,28])
    social = random.choice([1,2,3,4,5])
    gaming = random.choice(GAMING)
    mbti = random.choice(MBTI)
    n_veto = random.choices([1,2,3], weights=[50,35,15])[0]
    vetos = random.sample(VETO_ALL, n_veto)

    # Build raw_answers JSON (valid JSON for the algorithm)
    raw = f'{{"sleep":"{sleep}","hygiene":{hygiene},"mbti":"{mbti}"}}'

    qid = gen_id("q_")
    sql_questionnaire.append(
        f"INSERT INTO questionnaire (questionnaire_id, user_id, sleep_schedule, "
        f"hygiene_level, noise_tolerance, temperature_preference, social_preference, "
        f"gaming_behavior, mbti_type, raw_answers) VALUES ("
        f"'{qid}', '{esc(uid)}', '{sleep}', {hygiene}, {noise}, {temp}, {social}, "
        f"'{gaming}', '{mbti}', '{esc(raw)}');"
    )

    pref_id = f"pref_{uid}"
    sql_preference.append(
        f"INSERT INTO preference (preference_id, user_id, similarity_weight, "
        f"complementarity_weight, veto_safety_weight) VALUES ("
        f"'{pref_id}', '{esc(uid)}', "
        f"{round(random.uniform(0.3,0.6),2)}, "
        f"{round(random.uniform(0.1,0.3),2)}, "
        f"{round(random.uniform(0.1,0.4),2)}) "
        f"ON DUPLICATE KEY UPDATE user_id=user_id;"
    )

    for v in vetos:
        vid = gen_id("v_")
        sql_veto.append(
            f"INSERT INTO veto (veto_id, user_id, veto_item) VALUES ("
            f"'{vid}', '{esc(uid)}', '{v}');"
        )

    prof_id = f"prof_{uid}"
    sql_open_text.append(
        f"INSERT INTO open_text_profile (profile_id, user_id, "
        f"self_description, roommate_expectation) VALUES ("
        f"'{prof_id}', '{esc(uid)}', "
        f"'{esc(sno)}同学，{esc(college)}{esc(major)}，作息{sleep}型，MBTI为{mbti}。', "
        f"'{esc(random.choice(["希望室友爱干净尊重隐私","希望室友作息规律不吵闹","希望室友友好随和好相处"]))}') "
        f"ON DUPLICATE KEY UPDATE user_id=user_id;"
    )

# Output SQL file
with open("/tmp/batch_questionnaire.sql", "w") as f:
    for sqls in [sql_questionnaire, sql_preference, sql_veto, sql_open_text]:
        for s in sqls:
            f.write(s + "\n")

print(f"Generated SQL: {len(sql_questionnaire)} questionnaires, "
      f"{len(sql_preference)} preferences, "
      f"{len(sql_veto)} veto items, "
      f"{len(sql_open_text)} open texts")
print(f"Written to /tmp/batch_questionnaire.sql")
