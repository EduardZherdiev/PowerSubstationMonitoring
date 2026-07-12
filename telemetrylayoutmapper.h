#ifndef TELEMETRYLAYOUTMAPPER_H
#define TELEMETRYLAYOUTMAPPER_H

struct SensorSnapshot;

namespace SubstationLayout {
struct Layout;
}

namespace TelemetryLayoutMapper {

void apply(SubstationLayout::Layout *layout, const SensorSnapshot &snapshot);

} // namespace TelemetryLayoutMapper

#endif // TELEMETRYLAYOUTMAPPER_H
