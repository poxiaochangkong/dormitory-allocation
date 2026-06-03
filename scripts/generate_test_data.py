#!/usr/bin/env python3
"""
Test data generator for the dormitory allocation algorithm (v2.0).

Usage:
    python generate_test_data.py --seed    # Insert test data
    python generate_test_data.py --clean   # Remove all test data

All generated records use 'test_' prefixed IDs for safe cleanup.
"""

import argparse
import hashlib
import json
import os
import random
import secrets
import string
import sys

try:
    import mysql.connector
except ImportError:
    print("ERROR: mysql-connector-python not installed.")
    print("  pip install mysql-connector-python")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Config — matches backend/config/config.json
# ---------------------------------------------------------------------------
DB_CONFIG = {
    "host": "127.0.0.1",
    "port": 3306,
    "user": "root",
    "password": "root",
    "database": "dorm_alloc",
    "charset": "utf8mb4",
}

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
NUM_MALE = 20
NUM_FEMALE = 20
TOTAL_STUDENTS = NUM_MALE + NUM_FEMALE
DEFAULT_PASSWORD = "123456"

# All 16 MBTI types for full coverage
MBTI_TYPES = [
    "ISTJ", "ISFJ", "INFJ", "INTJ",
    "ISTP", "ISFP", "INFP", "INTP",
    "ESTP", "ESFP", "ENFP", "ENTP",
    "ESTJ", "ESFJ", "ENFJ", "ENTJ",
]

SLEEP_SCHEDULES = ["early", "normal", "late", "very_late"]
GAMING_BEHAVIORS = ["never", "sometimes", "often", "always"]
COLLEGES = ["计算机学院", "电子工程学院", "机械工程学院", "经济管理学院"]
MALES_MAJORS = ["软件工程", "计算机科学", "电子工程", "机械设计", "金融学"]
FEMALE_MAJORS = ["软件工程", "计算机科学", "通信工程", "会计学", "市场营销"]

# Buildings
MALE_BUILDINGS = ["A1", "A2", "A3"]
FEMALE_BUILDINGS = ["B1", "B2", "B3"]

VETO_ITEMS = [
    "smoke_alcohol",
    "midnight_gaming",
    "loud_speaker",
    "bad_hygiene",
    "over_demand",
    "boundary_violation",
    "pets",
    "opposite_sex",
]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def generate_id(prefix="test"):
    """Generate a unique ID with test_ prefix."""
    uid = secrets.token_hex(8)
    return f"{prefix}_{uid}"


def generate_salt():
    """Generate a 32-char hex salt (mimics CryptoUtil::GenerateSalt)."""
    return secrets.token_hex(16)


def hash_password(password: str, salt: str) -> str:
    """SHA-256(salt + password), matching CryptoUtil::HashPassword."""
    return hashlib.sha256((salt + password).encode("utf-8")).hexdigest()


def get_connection():
    return mysql.connector.connect(**DB_CONFIG)


# ---------------------------------------------------------------------------
# Seed data
# ---------------------------------------------------------------------------

def seed_data():
    conn = get_connection()
    cursor = conn.cursor()
    random.seed(42)  # Reproducible

    # Assign MBTI types — ensure all 16 types appear at least once
    mbti_pool = list(MBTI_TYPES)  # 16
    # Fill remaining slots with random picks
    while len(mbti_pool) < TOTAL_STUDENTS:
        mbti_pool.append(random.choice(MBTI_TYPES))
    random.shuffle(mbti_pool)

    # Generate student profiles
    students = []
    for i in range(TOTAL_STUDENTS):
        is_male = i < NUM_MALE
        gender = "male" if is_male else "female"
        college = random.choice(COLLEGES)
        major = random.choice(MALES_MAJORS if is_male else FEMALE_MAJORS)
        idx = i + 1

        students.append({
            "user_id": f"test_student_{idx:03d}",
            "student_no": f"2024{idx:04d}",
            "gender": gender,
            "college": college,
            "major": major,
            "grade": "2024",
            "mbti": mbti_pool[i],
            "sleep": random.choice(SLEEP_SCHEDULES),
            "hygiene": random.randint(1, 5),
            "noise": random.randint(1, 5),
            "temperature": random.choice([20, 22, 24, 26, 28]),
            "social": random.randint(1, 5),
            "gaming": random.choice(GAMING_BEHAVIORS),
        })

    # ---- Insert users ----
    salt = generate_salt()
    pw_hash = hash_password(DEFAULT_PASSWORD, salt)

    for s in students:
        cursor.execute(
            """INSERT IGNORE INTO `user`
               (user_id, student_no, password, salt, gender, college, major, grade, dorm_type, role)
               VALUES (%s, %s, %s, %s, %s, %s, %s, %s, 4, 'student')""",
            (s["user_id"], s["student_no"], pw_hash, salt,
             s["gender"], s["college"], s["major"], s["grade"]),
        )
    print(f"[OK] Inserted {len(students)} users")

    # ---- Insert questionnaires ----
    q_count = 0
    for s in students:
        qid = generate_id("test_q")
        raw_answers = json.dumps({
            "traditionalHabits": {
                "q10_smokeStatus": random.choice(["never", "sometimes", "often"]) if random.random() < 0.2 else "never",
                "q11_petPreference": random.choice(["have", "want", "none"]) if random.random() < 0.15 else "none",
                "q12_oppositeSex": random.choice(["often", "sometimes", "never"]) if random.random() < 0.1 else "never",
            }
        }, ensure_ascii=False)

        cursor.execute(
            """INSERT IGNORE INTO questionnaire
               (questionnaire_id, user_id, sleep_schedule, hygiene_level,
                noise_tolerance, temperature_preference, social_preference,
                gaming_behavior, mbti_type, raw_answers)
               VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)""",
            (qid, s["user_id"], s["sleep"], s["hygiene"],
             s["noise"], s["temperature"], s["social"],
             s["gaming"], s["mbti"], raw_answers),
        )
        q_count += 1
    print(f"[OK] Inserted {q_count} questionnaires")

    # ---- Insert preferences ----
    p_count = 0
    for s in students:
        pid = generate_id("test_pref")
        # Randomize weights slightly
        sim_w = round(random.uniform(0.3, 0.7), 2)
        comp_w = round(random.uniform(0.1, 0.4), 2)
        veto_w = round(max(0.1, 1.0 - sim_w - comp_w), 2)

        cursor.execute(
            """INSERT IGNORE INTO preference
               (preference_id, user_id, similarity_weight,
                complementarity_weight, veto_safety_weight)
               VALUES (%s, %s, %s, %s, %s)""",
            (pid, s["user_id"], sim_w, comp_w, veto_w),
        )
        p_count += 1
    print(f"[OK] Inserted {p_count} preferences")

    # ---- Insert vetos (for ~40% of students) ----
    veto_students = random.sample(students, k=int(TOTAL_STUDENTS * 0.4))
    v_count = 0
    for s in veto_students:
        # 1-3 veto items per student
        items = random.sample(VETO_ITEMS, k=random.randint(1, 3))
        for item in items:
            vid = generate_id("test_veto")
            cursor.execute(
                "INSERT IGNORE INTO veto (veto_id, user_id, veto_item) VALUES (%s, %s, %s)",
                (vid, s["user_id"], item),
            )
            v_count += 1
    print(f"[OK] Inserted {v_count} veto items for {len(veto_students)} students")

    # ---- Insert dormitories ----
    dorms = []
    dorm_count = 0

    # Male dorms
    for bldg in MALE_BUILDINGS:
        for room_num in range(101, 105):  # 4 rooms per building
            did = f"test_dorm_M_{bldg}_{room_num}"
            dorms.append(did)
            cursor.execute(
                """INSERT IGNORE INTO dormitory
                   (dorm_id, building, room_number, capacity, gender, is_available)
                   VALUES (%s, %s, %s, 4, 'male', 1)""",
                (did, bldg, str(room_num)),
            )
            dorm_count += 1

    # Female dorms
    for bldg in FEMALE_BUILDINGS:
        for room_num in range(201, 205):  # 4 rooms per building
            did = f"test_dorm_F_{bldg}_{room_num}"
            dorms.append(did)
            cursor.execute(
                """INSERT IGNORE INTO dormitory
                   (dorm_id, building, room_number, capacity, gender, is_available)
                   VALUES (%s, %s, %s, 4, 'female', 1)""",
                (did, bldg, str(room_num)),
            )
            dorm_count += 1

    print(f"[OK] Inserted {dorm_count} dormitories")

    # ---- Insert an allocation task ----
    task_id = "test_task_001"
    cursor.execute(
        """INSERT IGNORE INTO allocation_task
           (task_id, college, major, gender, status, rule_config)
           VALUES (%s, '', '', '', 'pending', %s)""",
        (task_id, json.dumps({
            "similarityWeight": 0.5,
            "complementarityWeight": 0.2,
            "vetoSafetyWeight": 0.3,
        })),
    )
    print(f"[OK] Inserted allocation task: {task_id}")

    conn.commit()
    cursor.close()
    conn.close()

    print()
    print("=" * 60)
    print("Test data seeded successfully!")
    print(f"  Students:  {TOTAL_STUDENTS} ({NUM_MALE}M + {NUM_FEMALE}F)")
    print(f"  MBTI types: all 16 covered")
    print(f"  Dormitories: {dorm_count} (capacity 4 each)")
    print(f"  Default password: {DEFAULT_PASSWORD}")
    print(f"  Task ID: {task_id}")
    print("=" * 60)
    print()
    print("To trigger allocation, start the server and call:")
    print(f"  POST /api/admin/run-allocation with task_id={task_id}")
    print()
    print("To clean up:")
    print("  python generate_test_data.py --clean")


# ---------------------------------------------------------------------------
# Clean up
# ---------------------------------------------------------------------------

def clean_data():
    conn = get_connection()
    cursor = conn.cursor()

    print("Cleaning test data...")

    # Delete in dependency order (respect foreign key constraints)
    # match_result has FK to user, dormitory, and task — clean thoroughly
    tables_to_clean = [
        ("match_result", "user_id LIKE 'test_%' OR dorm_id LIKE 'test_%' OR task_id LIKE 'test_%'"),
        ("algorithm_audit", "task_id LIKE 'test_%'"),
        ("allocation_task", "task_id LIKE 'test_%'"),
        ("veto", "user_id LIKE 'test_%'"),
        ("preference", "user_id LIKE 'test_%'"),
        ("questionnaire", "user_id LIKE 'test_%'"),
        ("open_text_profile", "user_id LIKE 'test_%'"),
        ("user", "user_id LIKE 'test_%'"),
        ("dormitory", "dorm_id LIKE 'test_%'"),
    ]

    total_deleted = 0
    for table, condition in tables_to_clean:
        cursor.execute(f"DELETE FROM `{table}` WHERE {condition}")
        deleted = cursor.rowcount
        total_deleted += deleted
        if deleted > 0:
            print(f"  [OK] Deleted {deleted} rows from `{table}`")

    conn.commit()
    cursor.close()
    conn.close()

    print()
    if total_deleted == 0:
        print("No test data found to clean.")
    else:
        print(f"Cleaned up {total_deleted} test records total.")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Test data generator for dormitory allocation algorithm")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--seed", action="store_true",
                       help="Insert test data into the database")
    group.add_argument("--clean", action="store_true",
                       help="Remove all test data from the database")
    args = parser.parse_args()

    try:
        if args.seed:
            seed_data()
        elif args.clean:
            clean_data()
    except mysql.connector.Error as e:
        print(f"Database error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()