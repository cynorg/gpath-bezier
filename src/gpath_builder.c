#include <pebble.h>
#include "gpath_builder.h"

const int fixedpoint_base = 16;

int32_t max_angle_tolerance = (TRIG_MAX_ANGLE / 360) * 10;

bool recursive_bezier_fixed(GPathBuilder *builder,
                            int32_t x1, int32_t y1,
                            int32_t x2, int32_t y2,
                            int32_t x3, int32_t y3,
                            int32_t x4, int32_t y4){



  // Calculate all the mid-points of the line segments
  int x12   = (x1 + x2) / 2;
  int y12   = (y1 + y2) / 2;
  int x23   = (x2 + x3) / 2;
  int y23   = (y2 + y3) / 2;
  int x34   = (x3 + x4) / 2;
  int y34   = (y3 + y4) / 2;
  int x123  = (x12 + x23) / 2;
  int y123  = (y12 + y23) / 2;
  int x234  = (x23 + x34) / 2;
  int y234  = (y23 + y34) / 2;
  int x1234 = (x123 + x234) / 2;
  int y1234 = (y123 + y234) / 2;

  // Angle Condition
  int32_t a23 = atan2_lookup((int16_t)((y3 - y2) / fixedpoint_base), (int16_t)((x3 - x2) / fixedpoint_base));
  int32_t da1 = abs(a23 - atan2_lookup((int16_t)((y2 - y1) / fixedpoint_base), (int16_t)((x2 - x1) / fixedpoint_base)));
  int32_t da2 = abs(atan2_lookup((int16_t)((y4 - y3) / fixedpoint_base), (int16_t)((x4 - x3) / fixedpoint_base)) - a23);
  if (da1 >= TRIG_MAX_ANGLE) {
    da1 = TRIG_MAX_ANGLE - da1;
  }
  if (da2 >= TRIG_MAX_ANGLE) {
    da2 = TRIG_MAX_ANGLE - da2;
  }

  if (da1 + da2 < max_angle_tolerance){
    // Finally we can stop the recursion
    return gpath_builder_line_to_point(builder, GPoint(x1234 / fixedpoint_base, y1234 / fixedpoint_base));
  }

  // Continue subdivision if points are being added successfully
  if (recursive_bezier_fixed(builder, x1, y1, x12, y12, x123, y123, x1234, y1234)
      && recursive_bezier_fixed(builder, x1234, y1234, x234, y234, x34, y34, x4, y4)) {
    return true;
  }
  return false;
}

bool bezier_fixed(GPathBuilder *builder, GPoint p1, GPoint p2, GPoint p3, GPoint p4){

  //translate points to fixedpoint realms
  int32_t x1 = p1.x * fixedpoint_base;
  int32_t x2 = p2.x * fixedpoint_base;
  int32_t x3 = p3.x * fixedpoint_base;
  int32_t x4 = p4.x * fixedpoint_base;
  int32_t y1 = p1.y * fixedpoint_base;
  int32_t y2 = p2.y * fixedpoint_base;
  int32_t y3 = p3.y * fixedpoint_base;
  int32_t y4 = p4.y * fixedpoint_base;

  if(recursive_bezier_fixed(builder, x1, y1, x2, y2, x3, y3, x4, y4)){
    return gpath_builder_line_to_point(builder, p4);
  }
  return false;
}

GPathBuilder *gpath_builder_create(uint32_t max_points) {
  const size_t required_size = sizeof(GPathBuilder) + max_points * sizeof(GPoint);
  GPathBuilder *result = malloc(required_size);
  memset(result, 0, required_size);
  result->max_points = max_points;
  return result;
}

void gpath_builder_destroy(GPathBuilder *builder) {
  free(builder);
}

GPath *gpath_builder_create_path(GPathBuilder *builder) {
  // TODO: make this using the official SDK setters
  // TODO: handle builder.next_point_index <= 1

  size_t num_points = builder->next_point_index;

  // handle case where last point == first point => remove last point
  while (num_points > 1 && gpoint_equal(&builder->points[0], &builder->points[num_points])) {
    num_points--;
  }

  const size_t size_of_points = num_points * sizeof(GPoint);
  GPath *result = malloc(sizeof(GPath) + size_of_points);
  result->points = builder->points;
  //memset(result, 0, sizeof(GPath)); //clears pointer address and results in memcpy failure
  result->num_points = num_points;
  memcpy(result->points, builder->points, size_of_points);
  return result;
}

bool gpath_builder_move_to_point(GPathBuilder *builder, GPoint to_point) {
  if (builder->next_point_index != 0) {
    return false;
  }

  return gpath_builder_line_to_point(builder, to_point);
}

bool gpath_builder_line_to_point(GPathBuilder *builder, GPoint to_point) {
  if (builder->next_point_index >= builder->max_points - 1) {
    return false;
  }

  builder->points[builder->next_point_index++] = to_point;
  return true;
}

bool gpath_builder_curve_to_point(GPathBuilder *builder, GPoint to_point, GPoint control_point_1, GPoint control_point_2) {
  GPoint from_point = builder->points[builder->next_point_index-1];
  return bezier_fixed(builder, from_point, control_point_1, control_point_2, to_point);
}