import 'package:flutter/material.dart';

/// Visual scaling belongs in Dart; Rust supplies measured spectrum magnitudes.
class SpectrumView extends StatelessWidget {
  const SpectrumView({super.key, required this.decibels});
  final List<double> decibels;
  @override
  Widget build(BuildContext context) => Semantics(
    label: 'Live audio spectrum',
    child: SizedBox(
      height: 14,
      child: CustomPaint(
        painter: _SpectrumPainter(
          decibels,
          Theme.of(context).colorScheme.primary,
        ),
        size: const Size(double.infinity, 14),
      ),
    ),
  );
}

class _SpectrumPainter extends CustomPainter {
  _SpectrumPainter(this.decibels, this.color);
  final List<double> decibels;
  final Color color;
  @override
  void paint(Canvas canvas, Size size) {
    if (decibels.isEmpty || size.width <= 0) {
      return;
    }
    final paint = Paint()..color = color;
    final width = size.width / decibels.length;
    for (var band = 0; band < decibels.length; ++band) {
      final db = decibels[band];
      final fraction = db.isFinite ? ((db + 80) / 80).clamp(0.0, 1.0) : 0.0;
      final height = size.height * fraction;
      canvas.drawRect(
        Rect.fromLTWH(band * width, size.height - height, width * .75, height),
        paint,
      );
    }
  }

  @override
  bool shouldRepaint(_SpectrumPainter old) =>
      old.decibels != decibels || old.color != color;
}
