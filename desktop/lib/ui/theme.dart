import 'package:flutter/material.dart';

ThemeData orangeTheme(Brightness brightness) {
  final scheme = ColorScheme.fromSeed(
    seedColor: const Color(0xffd96819),
    brightness: brightness,
  );
  return ThemeData(
    useMaterial3: true,
    colorScheme: scheme,
    visualDensity: VisualDensity.compact,
    materialTapTargetSize: MaterialTapTargetSize.shrinkWrap,
    inputDecorationTheme: const InputDecorationTheme(
      border: OutlineInputBorder(),
      isDense: true,
    ),
    tooltipTheme: const TooltipThemeData(
      waitDuration: Duration(milliseconds: 450),
    ),
    dividerTheme: DividerThemeData(color: scheme.outlineVariant),
    dataTableTheme: const DataTableThemeData(
      headingRowHeight: 36,
      dataRowMinHeight: 38,
      dataRowMaxHeight: 48,
    ),
  );
}

String durationLabel(int seconds) =>
    '${seconds ~/ 60}:${(seconds % 60).toString().padLeft(2, '0')}';
