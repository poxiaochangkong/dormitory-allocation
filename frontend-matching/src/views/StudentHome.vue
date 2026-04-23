<template>
  <div class="home-container">
    <el-card class="dashboard-card">
      <div class="welcome-header">
        <h2>👋 欢迎来到新生宿舍智能分配大厅</h2>
        <p>为了给您匹配最合拍的室友，请按顺序完成以下环节：</p>
      </div>

      <el-steps :active="1" align-center class="step-nav">
        <el-step title="阶段 1" description="基础画像与底线设置" />
        <el-step title="阶段 2" description="沉浸式生活习惯采集" />
        <el-step title="阶段 3" description="AI匹配计算与结果" />
      </el-steps>

      <div class="action-cards">
        <el-card shadow="hover" class="task-card">
          <h3>📝 问卷一：底线红线与基础画像</h3>
          <p>设置您绝对无法容忍的室友行为（一票否决），并完善您的基础作息。</p>
          <el-button type="primary" size="large" @click="$router.push('/student/questionnaire')">
            去填写
          </el-button>
        </el-card>

        <el-card shadow="hover" class="task-card">
          <h3>🏠 问卷二：沉浸式生活场景调查</h3>
          <p>进入虚拟宿舍环境，通过点击场景物品，告诉我们您最真实的生活细节。</p>
          <el-button type="warning" size="large" @click="$router.push('/student/scene')">
            去体验
          </el-button>
        </el-card>

        <el-card shadow="hover" class="task-card">
          <h3>🎉 查看最终结果</h3>
          <p>完成上述所有任务后，查看系统为您匹配的神仙室友名单！</p>
          <el-button type="success" size="large" @click="$router.push('/student/result')">
            查看分配报告
          </el-button>
        </el-card>
      </div>
    </el-card>
  </div>
</template>

<style scoped>
.home-container {
  padding: 40px 20px;
  display: flex;
  justify-content: center;
  min-height: 100vh;
}
.dashboard-card {
  width: 1000px;
  border-radius: 12px;
}
.welcome-header {
  text-align: center;
  margin-bottom: 40px;
}
.step-nav {
  margin-bottom: 50px;
}
.action-cards {
  display: flex;
  justify-content: space-around;
  gap: 20px;
}
.task-card {
  flex: 1;
  text-align: center;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
}
.task-card h3 { color: #303133; margin-bottom: 10px; }
.task-card p { color: #606266; font-size: 14px; margin-bottom: 20px; line-height: 1.6; }
</style>