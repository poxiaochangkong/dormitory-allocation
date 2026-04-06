<template>
  <div class="result-container">
    <el-card class="result-card">
      <template #header>
        <h2>🎉 您的宿舍分配结果已出炉</h2>
      </template>
      
      <el-descriptions title="分配详情" border :column="2">
        <el-descriptions-item label="宿舍号">南苑 4 栋 302</el-descriptions-item>
        <el-descriptions-item label="宿舍类型">4人间</el-descriptions-item>
        <el-descriptions-item label="室友名单">张三, 李四, 王五</el-descriptions-item>
        <el-descriptions-item label="匹配评级">
          <el-tag type="success" effect="dark">SSS 级完美匹配</el-tag>
        </el-descriptions-item>
      </el-descriptions>

      <div class="score-section">
        <h3>匹配算法各项得分评估</h3>
        <p>生活习惯相似性得分 (Sim) <el-progress :text-inside="true" :stroke-width="20" :percentage="92" status="success"/></p>
        <p>性格互补性得分 (Comp) <el-progress :text-inside="true" :stroke-width="20" :percentage="85" status="warning"/></p>
        <p>安全规避风险得分 (Safe) <el-progress :text-inside="true" :stroke-width="20" :percentage="98" status="exception"/></p>
      </div>

      <el-alert
        title="AI 匹配解释"
        type="info"
        description="系统基于您的问卷及一票否决项评估，您与当前室友在作息时间上高度一致，且均排斥深夜游戏噪音。室友中包含性格互补的 E 人与 I 人，能形成良好的寝室氛围。"
        show-icon
        class="mt-20"
      />
    </el-card>
  </div>
</template>

<style scoped>
.result-container { padding: 30px; display: flex; justify-content: center; background-color: #f5f7fa; min-height: 100vh; }
.result-card { width: 800px; }
.score-section { margin-top: 30px; }
.mt-20 { margin-top: 20px; }
</style>