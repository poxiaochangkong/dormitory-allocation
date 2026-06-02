#!/usr/bin/env python3
"""
重新生成问卷数据 — 包含 Phase 4 修复所需的新字段：
q04_noiseTolerance, q05_tempPref, q09_gamingHabit,
q10_smokeStatus, q11_petPreference, q12_oppositeSex
"""
import random, time, subprocess

random.seed(42)
MBTI = ["INTJ","INTP","ENTJ","ENTP","INFJ","INFP","ENFJ","ENFP",
        "ISTJ","ISFJ","ESTJ","ESFJ","ISTP","ISFP","ESTP","ESFP"]
SLEEP = ["early", "normal", "late"]
GAMING = ["never", "sometimes", "often", "always"]
GAMING_CODE = {"never":"A", "sometimes":"B", "often":"C", "always":"D"}
SMOKE = ["no", "sometimes", "often"]
PET = ["no", "want", "have"]
OPPSEX = ["never", "sometimes", "often"]
VETO_ALL = ["smoke_alcohol","midnight_gaming","loud_speaker","opposite_sex",
            "bad_hygiene","over_demand","boundary_violation","pets"]
WEIGHTS = [(3,7), (1,5), (1,4)]  # veto weight ranges

def gen_id(pref): return f"{pref}{int(time.time()*1000)}_{random.randint(1000,9999)}"

result = subprocess.run(
    ["docker", "exec", "mysql-dorm", "mysql", "-u", "root", "-proot", "dorm_alloc",
     "-N", "-e",
     "SELECT user_id, student_no, gender, college, major FROM `user` WHERE role='student' ORDER BY student_no"],
    capture_output=True, text=True)
lines = result.stdout.strip().split('\n')
students = []
for l in lines:
    p = l.split('\t')
    if len(p) >= 5: students.append({"uid":p[0],"sno":p[1],"gender":p[2],"college":p[3],"major":p[4]})
print(f"Processing {len(students)} students")

# DELETE old questionnaire/preference/veto data
subprocess.run(["docker","exec","mysql-dorm","mysql","-u","root","-proot","dorm_alloc",
    "-e","DELETE FROM veto; DELETE FROM preference; DELETE FROM match_result; DELETE FROM questionnaire; DELETE FROM allocation_task;"])
print("Cleaned old data")

# Generate SQL
sqls = []
for s in students:
    uid, sno, gender, college, major = s["uid"], s["sno"], s["gender"], s["college"], s["major"]
    sleep = random.choice(SLEEP)
    hygiene = random.choice([2,3,4,5])
    noise_tol = random.choice([1,2,3,4,5])
    temp_pref = random.choice([18,20,22,24,26,28,30])
    social = random.choice([1,2,3,4,5])
    gaming = random.choice(GAMING)
    gaming_c = GAMING_CODE[gaming]
    mbti = random.choice(MBTI)
    smoke = random.choices(SMOKE, weights=[60,25,15])[0]
    pet = random.choices(PET, weights=[80,15,5])[0]
    opp = random.choices(OPPSEX, weights=[70,25,5])[0]
    n_veto = random.choices([0,1,2,3], weights=[15,40,30,15])[0]
    vetos = random.sample(VETO_ALL, n_veto) if n_veto > 0 else []

    # Build raw_answers as frontend format
    raw = (
        f'{{"basicInfo":{{"gender":"{gender}","college":"{college}","major":"{major}"}},'
        f'"traditionalHabits":{{"q02_sleepTime":"{sleep[0].upper()}",'
        f'"q04_noiseTolerance":{noise_tol},"q05_tempPref":{temp_pref},'
        f'"q09_gamingHabit":"{gaming_c}","q10_smokeStatus":"{smoke}",'
        f'"q11_petPreference":"{pet}","q12_oppositeSex":"{opp}"}}}}'
    )

    sqls.append(
        f"INSERT INTO questionnaire (questionnaire_id,user_id,sleep_schedule,"
        f"hygiene_level,noise_tolerance,temperature_preference,social_preference,"
        f"gaming_behavior,mbti_type,raw_answers) VALUES ("
        f"'{gen_id('q_')}','{uid}','{sleep}',{hygiene},{noise_tol},{temp_pref},"
        f"{social},'{gaming}','{mbti}','{raw}');"
    )

    sim_w = round(random.uniform(0.3,0.6),2)
    comp_w = round(random.uniform(0.1,0.3),2)
    veto_w = round(1.0-sim_w-comp_w,2)
    sqls.append(
        f"INSERT INTO preference (preference_id,user_id,similarity_weight,"
        f"complementarity_weight,veto_safety_weight) VALUES ("
        f"'{gen_id('p_')}','{uid}',{sim_w},{comp_w},{veto_w}) "
        f"ON DUPLICATE KEY UPDATE user_id=user_id;"
    )

    for v in vetos:
        sqls.append(f"INSERT INTO veto (veto_id,user_id,veto_item) VALUES ('{gen_id('v_')}','{uid}','{v}');")

with open("/tmp/batch_q_v2.sql","w") as f:
    f.write("\n".join(sqls))

subprocess.run(["docker","exec","-i","mysql-dorm","mysql","-u","root","-proot","dorm_alloc"],
    input="\n".join(sqls), text=True)
print(f"Inserted {len(sqls)} SQL statements")
