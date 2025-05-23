#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "application/module/spk_controller_module.hpp"
#include "application/module/spk_keyboard_module.hpp"
#include "application/module/spk_module.hpp"
#include "application/module/spk_mouse_module.hpp"
#include "application/module/spk_paint_module.hpp"
#include "application/module/spk_system_module.hpp"
#include "application/module/spk_update_module.hpp"
#include "application/spk_application.hpp"
#include "application/spk_console_application.hpp"
#include "application/spk_graphical_application.hpp"
#include "external_libraries/stb_image.h"
#include "external_libraries/stb_truetype.h"
#include "spk_debug_macro.hpp"
#include "spk_sfinae.hpp"
#include "structure/container/spk_data_buffer.hpp"
#include "structure/container/spk_data_buffer_layout.hpp"
#include "structure/container/spk_json_file.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/container/spk_pool.hpp"
#include "structure/container/spk_thread_safe_queue.hpp"
#include "structure/design_pattern/spk_activable_object.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/design_pattern/spk_event_dispatcher.hpp"
#include "structure/design_pattern/spk_identifiable_object.hpp"
#include "structure/design_pattern/spk_inherence_object.hpp"
#include "structure/design_pattern/spk_observable_value.hpp"
#include "structure/design_pattern/spk_observer.hpp"
#include "structure/design_pattern/spk_singleton.hpp"
#include "structure/design_pattern/spk_stateful_object.hpp"
#include "structure/engine/spk_camera.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_directional_light.hpp"
#include "structure/engine/spk_game_engine.hpp"
#include "structure/engine/spk_game_object.hpp"
#include "structure/engine/spk_material.hpp"
#include "structure/engine/spk_mesh.hpp"
#include "structure/engine/spk_mesh_2D.hpp"
#include "structure/engine/spk_mesh_renderer.hpp"
#include "structure/engine/spk_mesh_renderer_2D.hpp"
#include "structure/engine/spk_transform.hpp"
#include "structure/engine/spk_tilemap.hpp"
#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_frame_buffer_object.hpp"
#include "structure/graphics/opengl/spk_index_buffer_object.hpp"
#include "structure/graphics/opengl/spk_layout_buffer_object.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"
#include "structure/graphics/opengl/spk_texture_object.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"
#include "structure/graphics/opengl/spk_vertex_array_object.hpp"
#include "structure/graphics/opengl/spk_vertex_buffer_object.hpp"
#include "structure/graphics/renderer/spk_color_renderer.hpp"
#include "structure/graphics/renderer/spk_font_renderer.hpp"
#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"
#include "structure/graphics/renderer/spk_texture_renderer.hpp"
#include "structure/graphics/spk_color.hpp"
#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/spk_text_alignment.hpp"
#include "structure/graphics/spk_viewport.hpp"
#include "structure/graphics/spk_window.hpp"
#include "structure/graphics/spk_pipeline.hpp"
#include "structure/graphics/texture/spk_font.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/math/spk_math.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_quaternion.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/network/spk_client.hpp"
#include "structure/network/spk_message.hpp"
#include "structure/network/spk_node_system.hpp"
#include "structure/network/spk_server.hpp"
#include "structure/spk_iostream.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/system/device/spk_controller.hpp"
#include "structure/system/device/spk_controller_input_thread.hpp"
#include "structure/system/device/spk_keyboard.hpp"
#include "structure/system/device/spk_mouse.hpp"
#include "structure/system/event/spk_event.hpp"
#include "structure/system/spk_boolean_enum.hpp"
#include "structure/system/spk_input_state.hpp"
#include "structure/system/time/spk_chronometer.hpp"
#include "structure/system/time/spk_timer.hpp"
#include "structure/thread/spk_persistant_worker.hpp"
#include "structure/thread/spk_thread.hpp"
#include "utils/spk_file_utils.hpp"
#include "utils/spk_opengl_utils.hpp"
#include "utils/spk_string_utils.hpp"
#include "utils/spk_system_utils.hpp"
#include "widget/spk_action_bar.hpp"
#include "widget/spk_container_widget.hpp"
#include "widget/spk_debug_overlay.hpp"
#include "widget/spk_frame.hpp"
#include "widget/spk_game_engine_widget.hpp"
#include "widget/spk_hud_widget.hpp"
#include "widget/spk_interface_window.hpp"
#include "widget/spk_push_button.hpp"
#include "widget/spk_screen.hpp"
#include "widget/spk_scroll_area.hpp"
#include "widget/spk_slider_bar.hpp"
#include "widget/spk_spin_box.hpp"
#include "widget/spk_text_edit.hpp"
#include "widget/spk_text_label.hpp"
#include "widget/spk_widget.hpp"
#include "widget/spk_layout.hpp"
#include "widget/spk_grid_layout.hpp"
#include "widget/spk_linear_layout.hpp"
#include "widget/spk_form_layout.hpp"
#include "widget/spk_spacer_widget.hpp"
#include "widget/spk_command_panel.hpp"
#include "widget/spk_text_area.hpp"
#include "widget/spk_message_box.hpp"