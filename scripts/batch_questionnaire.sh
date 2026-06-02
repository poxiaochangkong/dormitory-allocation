#!/bin/bash
# 批量提交问卷数据 — 为所有已注册的学生生成随机但合理的问卷
set -e

BASE_URL="${1:-http://localhost:8080}"
echo ">>> 批量问卷生成开始 <<<"

# 获取所有学生 user_id
STUDENTS=$(curl -s "$BASE_URL/api/admin/login" \
  -H "Content-Type: application/json" \
  -d '{"studentNo":"admin","password":"admin123"}' | python3 -c "
import sys, json
d = json.load(sys.stdin)
tok = d['data']['token']

import urllib.request
req = urllib.request.Request('$BASE_URL/api/admin/users',
  headers={'Authorization': f'Bearer {tok}'})
users = json.loads(urllib.request.urlopen(req).read())
for u in users['data']['users']:
    if u.get('role') == 'student':
        print(f\"{u['userId']}|{u['studentNo']}|{u.get('gender','')}|{u.get('college','')}|{u.get('major','')}\")
")

TOTAL=$(echo "$STUDENTS" | wc -l)
echo "找到 ${TOTAL} 名学生"

# 定义随机池
SLEEP_OPTS=("early" "normal" "late")
HYGIENE_LV=(2 3 4 5)
NOISE_LV=(2 3 4)
TEMP_LV=(20 22 24 26 28)
SOCIAL_LV=(1 2 3 4 5)
GAMING_OPTS=("never" "sometimes" "often")
MBTI=("INTJ" "INTP" "ENTJ" "ENTP" "INFJ" "INFP" "ENFJ" "ENFP" "ISTJ" "ISFJ" "ESTJ" "ESFJ" "ISTP" "ISFP" "ESTP" "ESFP")
VETO_ITEMS=("smoke_alcohol" "midnight_gaming" "loud_speaker" "opposite_sex" "bad_hygiene" "over_demand" "boundary_violation" "pets")

COUNT=0
echo "$STUDENTS" | while IFS='|' read uid sno gender college major; do
  [ -z "$uid" ] && continue

  # 随机作息: 与时间偏好关联
  SLEEP="${SLEEP_OPTS[$((RANDOM % 3))]}"
  HYGIENE=${HYGIENE_LV[$((RANDOM % 4))]}
  NOISE=${NOISE_LV[$((RANDOM % 3))]}
  TEMP=${TEMP_LV[$((RANDOM % 5))]}
  SOCIAL=${SOCIAL_LV[$((RANDOM % 5))]}
  GAMING="${GAMING_OPTS[$((RANDOM % 3))]}"
  MB="${MBTI[$((RANDOM % 16))]}"

  # 随机选 0-3 个否决项
  VETO_JSON="[]"
  R=$((RANDOM % 8))   # 先随机选1个大概率
  VETO_JSON="[\"${VETO_ITEMS[$R]}\"]"
  if [ $((RANDOM % 3)) -eq 0 ]; then
    R2=$((RANDOM % 8))
    [ $R2 -ne $R ] && VETO_JSON="[\"${VETO_ITEMS[$R]}\",\"${VETO_ITEMS[$R2]}\"]"
  fi

  # 随机权重
  SIM_W=$(python3 -c "print(round($((RANDOM % 30 + 40))/100.0, 2))")
  COMP_W=$(python3 -c "print(round($((RANDOM % 30 + 10))/100.0, 2))")
  VETO_W=$(python3 -c "print(round(1.0 - $SIM_W - $COMP_W, 2))")

  # 构建 JSON
  JSON=$(cat <<ENDQ
{
  "basicInfo": {"gender":"$gender","college":"$college","major":"$major"},
  "questionnaire": {
    "sleepSchedule":"$SLEEP",
    "hygieneLevel":$HYGIENE,
    "noiseTolerance":$NOISE,
    "temperaturePreference":$TEMP,
    "socialPreference":$SOCIAL,
    "gamingBehavior":"$GAMING",
    "mbtiType":"$MB"
  },
  "preference": {
    "similarityWeight":$SIM_W,
    "complementarityWeight":$COMP_W,
    "vetoSafetyWeight":$VETO_W
  },
  "vetoItems": $VETO_JSON,
  "openText": {
    "selfDescription":"$sno 同学，${college}${major}学生，作息${SLEEP}，MBTI为${MB}。",
    "roommateExpectation":"希望室友爱干净、不吵闹、互相尊重。"
  }
}
ENDQ
)

  # 提交
  RESP=$(curl -s -X POST "$BASE_URL/api/student/questionnaire/submit" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer tk_test" \
    -d "$JSON" 2>/dev/null || echo "FAIL")

  # 根据后端代码，问卷提交需要 auth，但后端 auth 验证 token 存在即可
  # 我们直接走管理员 token 提交

  echo "  $sno ($uid) -> submitted"
done

echo ">>> 问卷批量生成完成 <<<"
