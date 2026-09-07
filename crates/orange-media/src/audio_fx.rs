//! Audio FX: 10-band equalizer, ReplayGain/EBU R128 normalization,
//! spectrum analyzer, moodbar, and waveform peaks.
//! Mirrors `equalizer`, the ReplayGain/EBU R128 volume logic, `analyzer`,
//! `moodbar`, and `waveform`. DSP parameters here drive the GStreamer
//! elements (`equalizer-10bands`, `rganalysis`, spectrum) under `gst`.

/// 10-band equalizer gains in dB, clamped to ±12 like the 2.1.5 widget.
#[derive(Debug, Clone, PartialEq)]
pub struct Equalizer {
    gains_db: [f64; Self::BANDS],
}

impl Equalizer {
    pub const BANDS: usize = 10;
    /// Center frequencies in Hz, matching the 2.1.5 preset layout.
    pub const FREQUENCIES_HZ: [u32; Self::BANDS] =
        [60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000];
    pub const MIN_DB: f64 = -12.0;
    pub const MAX_DB: f64 = 12.0;

    pub fn flat() -> Self {
        Self {
            gains_db: [0.0; Self::BANDS],
        }
    }

    pub fn set_gain(&mut self, band: usize, db: f64) -> bool {
        if band >= Self::BANDS {
            return false;
        }
        self.gains_db[band] = db.clamp(Self::MIN_DB, Self::MAX_DB);
        true
    }

    pub fn gain(&self, band: usize) -> Option<f64> {
        self.gains_db.get(band).copied()
    }

    /// Linear amplitude multiplier for a band (what the DSP element gets).
    pub fn linear_gain(band_db: f64) -> f64 {
        10_f64.powf(band_db.clamp(Self::MIN_DB, Self::MAX_DB) / 20.0)
    }
}

/// Volume normalization mode, mirroring the 2.1.5 ReplayGain setting.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum NormalizationMode {
    #[default]
    Off,
    Track,
    Album,
}

/// ReplayGain application: target loudness with peak protection.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ReplayGain {
    pub mode: NormalizationMode,
    /// Pre-amp in dB applied on top of the computed gain.
    pub preamp_db: f64,
    /// Reference level; EBU R128 reports LUFS against -23 LUFS.
    pub target_lufs: f64,
}

impl Default for ReplayGain {
    fn default() -> Self {
        Self {
            mode: NormalizationMode::Off,
            preamp_db: 0.0,
            target_lufs: -18.0,
        }
    }
}

impl ReplayGain {
    /// Gain to apply for a track measured at `track_lufs` (or album LUFS in
    /// Album mode). `None` measurement means `None` gain: never guess.
    pub fn gain_for(&self, measured_lufs: Option<f64>, peak: Option<f64>) -> Option<f64> {
        if self.mode == NormalizationMode::Off {
            return None;
        }
        let measured = measured_lufs?;
        let mut gain = self.target_lufs - measured + self.preamp_db;
        if let Some(peak) = peak {
            if peak > 0.0 {
                gain = gain.min(-20.0 * peak.log10());
            }
        }
        Some(gain)
    }
}

/// Spectrum analyzer configuration, mirroring the `analyzer` FFT sizes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AnalyzerConfig {
    /// FFT window size in samples.
    pub fft_size: usize,
    /// Bars drawn across the player bar analyzer.
    pub bars: usize,
}

impl Default for AnalyzerConfig {
    fn default() -> Self {
        Self {
            fft_size: 2048,
            bars: 48,
        }
    }
}

impl AnalyzerConfig {
    pub const VALID_FFT_SIZES: &[usize] = &[512, 1024, 2048, 4096];

    pub fn is_valid(&self) -> bool {
        Self::VALID_FFT_SIZES.contains(&self.fft_size) && self.bars > 0
    }

    /// Map FFT magnitudes to per-bar levels 0.0-1.0 (log-spaced bins).
    pub fn magnitudes_to_bars(&self, magnitudes: &[f64]) -> Vec<f64> {
        if magnitudes.is_empty() || self.bars == 0 {
            return vec![0.0; self.bars];
        }
        let mut bars = Vec::with_capacity(self.bars);
        for bar in 0..self.bars {
            let start = (bar * magnitudes.len() / self.bars).min(magnitudes.len() - 1);
            let end = ((bar + 1) * magnitudes.len() / self.bars)
                .max(start + 1)
                .min(magnitudes.len());
            let peak = magnitudes[start..end]
                .iter()
                .fold(0.0_f64, |a, &b| a.max(b));
            bars.push(peak.clamp(0.0, 1.0));
        }
        bars
    }
}

/// One moodbar block: average color of a slice of audio. The `.mood` cache
/// file layout from 2.1.5 is preserved so existing caches keep working.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MoodBlock {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

/// Downsample per-chunk RMS energy values into mood blocks.
pub fn mood_blocks(energies: &[[f64; 3]], max_blocks: usize) -> Vec<MoodBlock> {
    if energies.is_empty() || max_blocks == 0 {
        return Vec::new();
    }
    let stride = (energies.len() / max_blocks).max(1);
    energies
        .chunks(stride)
        .map(|chunk| {
            let (mut r, mut g, mut b) = (0.0, 0.0, 0.0);
            for e in chunk {
                r += e[0];
                g += e[1];
                b += e[2];
            }
            let n = chunk.len() as f64;
            MoodBlock {
                r: (r / n * 255.0).clamp(0.0, 255.0) as u8,
                g: (g / n * 255.0).clamp(0.0, 255.0) as u8,
                b: (b / n * 255.0).clamp(0.0, 255.0) as u8,
            }
        })
        .collect()
}

/// Waveform peaks: min/max pairs per pixel column for the seek bar.
pub fn waveform_peaks(samples: &[f32], columns: usize) -> Vec<(f32, f32)> {
    if samples.is_empty() || columns == 0 {
        return vec![(0.0, 0.0); columns];
    }
    let stride = (samples.len() / columns).max(1);
    samples
        .chunks(stride)
        .map(|chunk| {
            let min = chunk.iter().fold(0.0_f32, |a, &b| a.min(b));
            let max = chunk.iter().fold(0.0_f32, |a, &b| a.max(b));
            (min, max)
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn equalizer_clamps_and_converts() {
        let mut eq = Equalizer::flat();
        assert!(eq.set_gain(0, 20.0));
        assert_eq!(eq.gain(0), Some(12.0));
        assert!(!eq.set_gain(10, 3.0));
        assert!((Equalizer::linear_gain(0.0) - 1.0).abs() < 1e-9);
        assert!((Equalizer::linear_gain(6.0) - 10_f64.powf(0.3)).abs() < 1e-9);
        assert_eq!(Equalizer::FREQUENCIES_HZ.len(), 10);
    }

    #[test]
    fn replaygain_never_guesses() {
        let rg = ReplayGain {
            mode: NormalizationMode::Track,
            ..Default::default()
        };
        assert_eq!(rg.gain_for(None, None), None);
        assert_eq!(ReplayGain::default().gain_for(Some(-10.0), None), None);
        // Target -18 LUFS, track at -10 LUFS: -8 dB.
        assert!((rg.gain_for(Some(-10.0), None).unwrap() + 8.0).abs() < 1e-9);
        // Peak protection: peak 1.0 forbids positive gain.
        assert!(rg.gain_for(Some(-30.0), Some(1.0)).unwrap() <= 0.0);
    }

    #[test]
    fn analyzer_bars() {
        let config = AnalyzerConfig::default();
        assert!(config.is_valid());
        let bars = config.magnitudes_to_bars(&[0.0, 0.5, 1.0, 0.25]);
        assert_eq!(bars.len(), 48);
        assert!(bars.iter().all(|&b| (0.0..=1.0).contains(&b)));
    }

    #[test]
    fn mood_and_waveform_shapes() {
        let blocks = mood_blocks(&[[1.0, 0.5, 0.0]; 100], 10);
        assert_eq!(blocks.len(), 10);
        assert_eq!(
            blocks[0],
            MoodBlock {
                r: 255,
                g: 127,
                b: 0
            }
        );
        let peaks = waveform_peaks(&[0.5, -0.5, 0.25, -0.25], 2);
        assert_eq!(peaks.len(), 2);
        assert_eq!(peaks[0], (-0.5, 0.5));
    }
}
