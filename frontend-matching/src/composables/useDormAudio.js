// Web Audio API — 宿舍环境音效
let audioCtx = null
let noiseNode = null
let acHumNode = null
let gainNode = null
let filterNode = null
let isPlaying = false

export function useDormAudio() {
  const init = () => {
    if (audioCtx) return
    audioCtx = new (window.AudioContext || window.webkitAudioContext)()

    // 主增益
    gainNode = audioCtx.createGain()
    gainNode.gain.value = 0.08
    gainNode.connect(audioCtx.destination)

    // 低通滤波（模拟隔墙）
    filterNode = audioCtx.createBiquadFilter()
    filterNode.type = 'lowpass'
    filterNode.frequency.value = 800
    filterNode.connect(gainNode)

    // 棕噪声（空调/风扇低频嗡嗡声）
    const bufferSize = 2 * audioCtx.sampleRate
    const buffer = audioCtx.createBuffer(1, bufferSize, audioCtx.sampleRate)
    const data = buffer.getChannelData(0)
    let b0 = 0, b1 = 0, b2 = 0
    for (let i = 0; i < bufferSize; i++) {
      const white = Math.random() * 2 - 1
      b0 = 0.99765 * b0 + white * 0.099046
      b1 = 0.963 * b1 + white * 0.2965164
      b2 = 0.570 * b2 + white * 1.052691
      data[i] = (b0 + b1 + b2 + white * 0.1848) * 0.08
    }
    noiseNode = audioCtx.createBufferSource()
    noiseNode.buffer = buffer
    noiseNode.loop = true
    noiseNode.connect(filterNode)

    // 50Hz 电源嗡嗡声
    acHumNode = audioCtx.createOscillator()
    acHumNode.type = 'sine'
    acHumNode.frequency.value = 50
    const humGain = audioCtx.createGain()
    humGain.gain.value = 0.03
    acHumNode.connect(humGain)
    humGain.connect(filterNode)

    noiseNode.start()
    acHumNode.start()
    isPlaying = true
  }

  const setVolume = (v) => {
    if (gainNode) gainNode.gain.value = v * 0.15
  }

  const setFilter = (freq) => {
    // 夜间模式：降低截止频率，模拟隔墙听觉
    if (filterNode) filterNode.frequency.value = freq
  }

  const toggleNight = (isNight) => {
    setFilter(isNight ? 400 : 1200)
    setVolume(isNight ? 0.3 : 0.6)
  }

  const playTap = () => {
    if (!audioCtx) return
    // 短促敲击声
    const osc = audioCtx.createOscillator()
    const g = audioCtx.createGain()
    osc.type = 'sine'; osc.frequency.value = 800
    g.gain.setValueAtTime(0.3, audioCtx.currentTime)
    g.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + 0.05)
    osc.connect(g); g.connect(audioCtx.destination)
    osc.start(); osc.stop(audioCtx.currentTime + 0.05)
  }

  const playClick = () => {
    if (!audioCtx) return
    const osc = audioCtx.createOscillator()
    const g = audioCtx.createGain()
    osc.type = 'square'; osc.frequency.value = 1200
    g.gain.setValueAtTime(0.15, audioCtx.currentTime)
    g.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + 0.03)
    osc.connect(g); g.connect(audioCtx.destination)
    osc.start(); osc.stop(audioCtx.currentTime + 0.03)
  }

  const dispose = () => {
    if (noiseNode) { try { noiseNode.stop() } catch(e) {} }
    if (acHumNode) { try { acHumNode.stop() } catch(e) {} }
    if (audioCtx) { audioCtx.close() }
    audioCtx = null; isPlaying = false
  }

  return { init, setVolume, toggleNight, playTap, playClick, dispose, isPlaying: () => isPlaying }
}
