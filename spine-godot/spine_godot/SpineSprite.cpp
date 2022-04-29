/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated January 1, 2020. Replaces all prior versions.
 *
 * Copyright (c) 2013-2020, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software
 * or otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THE SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include "SpineSprite.h"
#include "SpineEvent.h"
#include "SpineTrackEntry.h"
#include "SpineSkeleton.h"
#include "SpineRendererObject.h"
#include "SpineSlotNode.h"

Ref<CanvasItemMaterial> SpineSprite::default_materials[4] = {};
static int sprite_count = 0;
static spine::Vector<unsigned short> quad_indices;
static spine::Vector<float> scratch_vertices;
static Vector<Vector2> scratch_points;
static Vector<Vector2> scratch_uvs;
static Vector<Color> scratch_colors;
static Vector<int> scratch_indices;

void SpineSprite::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_skeleton_data_res", "skeleton_data_res"), &SpineSprite::set_skeleton_data_res);
	ClassDB::bind_method(D_METHOD("get_skeleton_data_res"), &SpineSprite::get_skeleton_data_res);
	ClassDB::bind_method(D_METHOD("get_skeleton"), &SpineSprite::get_skeleton);
	ClassDB::bind_method(D_METHOD("get_animation_state"), &SpineSprite::get_animation_state);
	ClassDB::bind_method(D_METHOD("on_skeleton_data_changed"), &SpineSprite::on_skeleton_data_changed);

	ClassDB::bind_method(D_METHOD("get_global_bone_transform", "bone_name"), &SpineSprite::get_global_bone_transform);
	ClassDB::bind_method(D_METHOD("set_global_bone_transform", "bone_name", "global_transform"), &SpineSprite::set_global_bone_transform);

	ClassDB::bind_method(D_METHOD("set_update_mode", "v"), &SpineSprite::set_update_mode);
	ClassDB::bind_method(D_METHOD("get_update_mode"), &SpineSprite::get_update_mode);

	ClassDB::bind_method(D_METHOD("set_normal_material", "material"), &SpineSprite::set_normal_material);
	ClassDB::bind_method(D_METHOD("get_normal_material"), &SpineSprite::get_normal_material);
	ClassDB::bind_method(D_METHOD("set_additive_material", "material"), &SpineSprite::set_additive_material);
	ClassDB::bind_method(D_METHOD("get_additive_material"), &SpineSprite::get_additive_material);
	ClassDB::bind_method(D_METHOD("set_multiply_material", "material"), &SpineSprite::set_multiply_material);
	ClassDB::bind_method(D_METHOD("get_multiply_material"), &SpineSprite::get_multiply_material);
	ClassDB::bind_method(D_METHOD("set_screen_material", "material"), &SpineSprite::set_screen_material);
	ClassDB::bind_method(D_METHOD("get_screen_material"), &SpineSprite::get_screen_material);

	ClassDB::bind_method(D_METHOD("update_skeleton", "delta"), &SpineSprite::update_skeleton);
	ClassDB::bind_method(D_METHOD("new_skin", "name"), &SpineSprite::new_skin);

	ADD_SIGNAL(MethodInfo("animation_started", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite"), PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry")));
	ADD_SIGNAL(MethodInfo("animation_interrupted", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite"), PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry")));
	ADD_SIGNAL(MethodInfo("animation_ended", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite"), PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry")));
	ADD_SIGNAL(MethodInfo("animation_completed", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite"), PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry")));
	ADD_SIGNAL(MethodInfo("animation_disposed", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite"), PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry")));
	ADD_SIGNAL(MethodInfo("animation_event", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite"), PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry"), PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_TYPE_STRING, "SpineEvent")));
	ADD_SIGNAL(MethodInfo("before_animation_state_update", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite")));
	ADD_SIGNAL(MethodInfo("before_animation_state_apply", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite")));
	ADD_SIGNAL(MethodInfo("before_world_transforms_change", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite")));
	ADD_SIGNAL(MethodInfo("world_transforms_changed", PropertyInfo(Variant::OBJECT, "spine_sprite", PROPERTY_HINT_TYPE_STRING, "SpineSprite")));
	ADD_SIGNAL(MethodInfo("_internal_spine_objects_invalidated"));

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "skeleton_data_res", PropertyHint::PROPERTY_HINT_RESOURCE_TYPE, "SpineSkeletonDataResource"), "set_skeleton_data_res", "get_skeleton_data_res");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "update_mode", PROPERTY_HINT_ENUM, "Process,Physics,Manual"), "set_update_mode", "get_update_mode");
	ADD_GROUP("Materials", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "normal_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_normal_material", "get_normal_material");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "additive_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_additive_material", "get_additive_material");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "multiply_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_multiply_material", "get_multiply_material");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "screen_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_screen_material", "get_screen_material");

	BIND_ENUM_CONSTANT(UpdateMode::UpdateMode_Process)
	BIND_ENUM_CONSTANT(UpdateMode::UpdateMode_Physics)
	BIND_ENUM_CONSTANT(UpdateMode::UpdateMode_Manual)
}

SpineSprite::SpineSprite() : update_mode(UpdateMode_Process), skeleton_clipper(nullptr) {
	skeleton_clipper = new spine::SkeletonClipping();

	// One material per blend mode, shared across all sprites.
	if (!default_materials[0].is_valid()) {
		Ref<CanvasItemMaterial> material_normal(memnew(CanvasItemMaterial));
		material_normal->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MIX);
		default_materials[spine::BlendMode_Normal] = material_normal;

		Ref<CanvasItemMaterial> material_additive(memnew(CanvasItemMaterial));
		material_additive->set_blend_mode(CanvasItemMaterial::BLEND_MODE_ADD);
		default_materials[spine::BlendMode_Additive] = material_additive;

		Ref<CanvasItemMaterial> material_multiply(memnew(CanvasItemMaterial));
		material_multiply->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MUL);
		default_materials[spine::BlendMode_Multiply] = material_multiply;

		Ref<CanvasItemMaterial> material_screen(memnew(CanvasItemMaterial));
		material_screen->set_blend_mode(CanvasItemMaterial::BLEND_MODE_SUB);
		default_materials[spine::BlendMode_Screen] = material_screen;
	}

	// Setup static scratch buffers
	if (quad_indices.size() == 0) {
		quad_indices.setSize(6, 0);
		quad_indices[0] = 0;
		quad_indices[1] = 1;
		quad_indices[2] = 2;
		quad_indices[3] = 2;
		quad_indices[4] = 3;
		quad_indices[5] = 0;
		scratch_vertices.ensureCapacity(1200);
	}
	sprite_count++;
}

SpineSprite::~SpineSprite() {
	delete skeleton_clipper;
	sprite_count--;
	if (!sprite_count) {
		for (int i = 0; i < 4; i++)
			default_materials[i].unref();
	}
}

void SpineSprite::set_skeleton_data_res(const Ref<SpineSkeletonDataResource> &_skeleton_data) {
	skeleton_data_res = _skeleton_data;
	on_skeleton_data_changed();
}
Ref<SpineSkeletonDataResource> SpineSprite::get_skeleton_data_res() {
	return skeleton_data_res;
}

void SpineSprite::on_skeleton_data_changed() {
	remove_meshes();
	skeleton.unref();
	animation_state.unref();
	emit_signal("_internal_spine_objects_invalidated");

	if (skeleton_data_res.is_valid()) {
#if VERSION_MAJOR > 3
		if (!skeleton_data_res->is_connected("skeleton_data_changed", callable_mp(this, &SpineSprite::on_skeleton_data_changed)))
			skeleton_data_res->connect("skeleton_data_changed", callable_mp(this, &SpineSprite::on_skeleton_data_changed));
#else
		if (!skeleton_data_res->is_connected("skeleton_data_changed", this, "on_skeleton_data_changed"))
			skeleton_data_res->connect("skeleton_data_changed", this, "on_skeleton_data_changed");
#endif
	}

	if (skeleton_data_res.is_valid() && skeleton_data_res->is_skeleton_data_loaded()) {
		skeleton = Ref<SpineSkeleton>(memnew(SpineSkeleton));
		skeleton->set_spine_sprite(this);
		skeleton->get_spine_object()->setScaleY(-1);

		animation_state = Ref<SpineAnimationState>(memnew(SpineAnimationState));
		animation_state->set_spine_sprite(this);
		animation_state->get_spine_object()->setListener(this);

		animation_state->update(0);
		animation_state->apply(skeleton);
		skeleton->update_world_transform();
		generate_meshes_for_slots(skeleton);

		if (update_mode == UpdateMode_Process) {
			_notification(NOTIFICATION_INTERNAL_PROCESS);
		} else if (update_mode == UpdateMode_Physics) {
			_notification(NOTIFICATION_INTERNAL_PHYSICS_PROCESS);
		}
	}

	NOTIFY_PROPERTY_LIST_CHANGED();
}

void SpineSprite::generate_meshes_for_slots(Ref<SpineSkeleton> skeleton_ref) {
	auto skeleton = skeleton_ref->get_spine_object();
	for (int i = 0, n = (int)skeleton->getSlots().size(); i < n; i++) {
		auto mesh_instance = memnew(MeshInstance2D);
		mesh_instance->set_position(Vector2(0, 0));
		mesh_instance->set_material(default_materials[spine::BlendMode_Normal]);
		add_child(mesh_instance);
		mesh_instances.push_back(mesh_instance);
		slot_nodes.add(spine::Vector<SpineSlotNode*>());
	}
}

void SpineSprite::remove_meshes() {
	for (int i = 0; i < mesh_instances.size(); ++i) {
		remove_child(mesh_instances[i]);
		memdelete(mesh_instances[i]);
	}
	mesh_instances.clear();
	slot_nodes.clear();
}

void SpineSprite::sort_slot_nodes() {
	for (int i = 0; i < (int)slot_nodes.size(); i++) {
		slot_nodes[i].setSize(0, nullptr);
	}
	
	auto draw_order = skeleton->get_spine_object()->getDrawOrder();
	for (int i = 0; i < get_child_count(); i++) {
		auto slot_node = Object::cast_to<SpineSlotNode>(get_child(i));
		if (!slot_node) continue;
		if (slot_node->get_slot_index() == -1 || slot_node->get_slot_index() >= (int)draw_order.size()) {
			continue;
		}
		slot_nodes[slot_node->get_slot_index()].add(slot_node);
	}
	
	for (int i = 0; i < (int)draw_order.size(); i++) {
		int slot_index = draw_order[i]->getData().getIndex();
		int mesh_index = mesh_instances[i]->get_index();
		spine::Vector<SpineSlotNode*> &nodes = slot_nodes[slot_index];
		for (int j = 0; j < (int)nodes.size(); j++) {
			auto node = nodes[j];
			move_child(node, mesh_index + 1);
		}
	}
}

Ref<SpineSkeleton> SpineSprite::get_skeleton() {
	return skeleton;
}

Ref<SpineAnimationState> SpineSprite::get_animation_state() {
	return animation_state;
}

void SpineSprite::_notification(int what) {
	switch (what) {
		case NOTIFICATION_READY: {
			set_process_internal(update_mode == UpdateMode_Process);
			set_physics_process_internal(update_mode == UpdateMode_Physics);
			break;
		}
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (update_mode == UpdateMode_Process)
				update_skeleton(get_process_delta_time());
			break;
		}
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (update_mode == UpdateMode_Physics)
				update_skeleton(get_physics_process_delta_time());
			break;
		}
		default:
			break;
	}
}

void SpineSprite::update_skeleton(float delta) {
	if (!skeleton_data_res.is_valid() ||
		!skeleton_data_res->is_skeleton_data_loaded() ||
		!skeleton.is_valid() ||
		!skeleton->get_spine_object() ||
		!animation_state.is_valid() ||
		!animation_state->get_spine_object())
		return;

	emit_signal("before_animation_state_update", this);
	animation_state->update(delta);
	if (!is_visible_in_tree()) return;
	emit_signal("before_animation_state_apply", this);
	animation_state->apply(skeleton);
	emit_signal("before_world_transforms_change", this);
	skeleton->update_world_transform();
	emit_signal("world_transforms_changed", this);
	sort_slot_nodes();
	update_meshes(skeleton);
	update();
}

static void clear_mesh_instance(MeshInstance2D *mesh_instance) {
#if VERSION_MAJOR > 3
	RenderingServer::get_singleton()->canvas_item_clear(mesh_instance->get_canvas_item());
#else
	VisualServer::get_singleton()->canvas_item_clear(mesh_instance->get_canvas_item());
#endif
}

static void add_triangles(MeshInstance2D *mesh_instance,
	const Vector<Point2> &vertices,
	const Vector<Point2> &uvs,
	const Vector<Color> &colors,
	const Vector<int> &indices,
	Ref<Texture> texture,
	Ref<Texture> normal_map) {
#if VERSION_MAJOR > 3
	RenderingServer::get_singleton()->canvas_item_add_triangle_array(mesh_ins->get_canvas_item(),
																  indices,
																  vertices,
																  colors,
																  uvs,
																  Vector<int>(),
																  Vector<float>(),
																  tex.is_null() ? RID() : tex->get_rid(),
																  -1);
#else
	VisualServer::get_singleton()->canvas_item_add_triangle_array(mesh_instance->get_canvas_item(),
																  indices,
																  vertices,
																  colors,
																  uvs,
																  Vector<int>(),
																  Vector<float>(),
																  texture.is_null() ? RID() : texture->get_rid(),
																  -1,
																  normal_map.is_null() ? RID() : normal_map->get_rid());
#endif
}

void SpineSprite::update_meshes(Ref<SpineSkeleton> skeleton_ref) {
	spine::Skeleton *skeleton = skeleton_ref->get_spine_object();
	for (int i = 0, n = (int)skeleton->getSlots().size(); i < n; ++i) {
		spine::Slot *slot = skeleton->getDrawOrder()[i];
		spine::Attachment *attachment = slot->getAttachment();
		MeshInstance2D *mesh_instance = mesh_instances[i];
		if (!attachment) {
			mesh_instances[i]->set_visible(false);
			skeleton_clipper->clipEnd(*slot);
			continue;
		}
		mesh_instance->set_visible(true);
		clear_mesh_instance(mesh_instance);

		spine::Color skeleton_color = skeleton->getColor();
		spine::Color slot_color = slot->getColor();
		spine::Color tint(skeleton_color.r * slot_color.r, skeleton_color.g * slot_color.g, skeleton_color.b * slot_color.b, skeleton_color.a * slot_color.a);
		Ref<Texture> texture;
		Ref<Texture> normal_map;
		spine::Vector<float> *vertices = &scratch_vertices;
		spine::Vector<float> *uvs;
		spine::Vector<unsigned short> *indices;

		if (attachment->getRTTI().isExactly(spine::RegionAttachment::rtti)) {
			auto *region = (spine::RegionAttachment *) attachment;
			auto renderer_object = (SpineRendererObject *) ((spine::AtlasRegion *) region->getRendererObject())->page->getRendererObject();
			texture = renderer_object->texture;
			normal_map = renderer_object->normal_map;
			
			vertices->setSize(8, 0);
			region->computeWorldVertices(*slot, *vertices, 0);
			uvs = &region->getUVs();
			indices = &quad_indices;

			auto attachment_color = region->getColor();
			tint.r *= attachment_color.r;
			tint.g *= attachment_color.g;
			tint.b *= attachment_color.b;
			tint.a *= attachment_color.a;
		} else if (attachment->getRTTI().isExactly(spine::MeshAttachment::rtti)) {
			auto *mesh = (spine::MeshAttachment *) attachment;
			auto renderer_object = (SpineRendererObject *) ((spine::AtlasRegion *) mesh->getRendererObject())->page->getRendererObject();
			texture = renderer_object->texture;
			normal_map = renderer_object->normal_map;
			
			vertices->setSize(mesh->getWorldVerticesLength(), 0);
			mesh->computeWorldVertices(*slot, *vertices);
			uvs = &mesh->getUVs();
			indices = &mesh->getTriangles();

			auto attachment_color = mesh->getColor();
			tint.r *= attachment_color.r;
			tint.g *= attachment_color.g;
			tint.b *= attachment_color.b;
			tint.a *= attachment_color.a;
		} else if (attachment->getRTTI().isExactly(spine::ClippingAttachment::rtti)) {
			auto clip = (spine::ClippingAttachment *) attachment;
			skeleton_clipper->clipStart(*slot, clip);
			continue;
		} else {
			skeleton_clipper->clipEnd(*slot);
			continue;
		}

		if (skeleton_clipper->isClipping()) {
			skeleton_clipper->clipTriangles(*vertices, *indices, *uvs, 2);
			if (skeleton_clipper->getClippedTriangles().size() == 0) {
				skeleton_clipper->clipEnd(*slot);
				continue;
			}

			vertices = &skeleton_clipper->getClippedVertices();
			uvs = &skeleton_clipper->getClippedUVs();
			indices = &skeleton_clipper->getClippedTriangles();
		}
		
		if (indices->size() > 0) {
			size_t num_vertices = vertices->size() / 2;
			scratch_points.resize((int)num_vertices);
			memcpy(scratch_points.ptrw(), vertices->buffer(), num_vertices * 2 * sizeof(float));
			scratch_uvs.resize((int)num_vertices);
			memcpy(scratch_uvs.ptrw(), uvs->buffer(), num_vertices * 2 * sizeof(float));
			scratch_colors.resize((int)num_vertices);
			for (int j = 0; j < (int)num_vertices; j++) {
				scratch_colors.set(j, Color(tint.r, tint.g, tint.b, tint.a));
			}
			scratch_indices.resize((int)indices->size());
			for (int j = 0; j < (int)indices->size(); ++j) {
				scratch_indices.set(j, indices->buffer()[j]);
			}

			add_triangles(mesh_instance, scratch_points, scratch_uvs, scratch_colors, scratch_indices, texture, normal_map);

			spine::BlendMode blend_mode = slot->getData().getBlendMode();
			Ref<Material> custom_material;

			// See if we have a slot node for this slot with a custom material
			auto &nodes = slot_nodes[i];
			if (nodes.size() > 0) {
				auto slot_node = nodes[0];
				if (slot_node) {
					switch (blend_mode) {
					case spine::BlendMode_Normal: custom_material = slot_node->get_normal_material(); break;
					case spine::BlendMode_Additive: custom_material = slot_node->get_additive_material(); break;
					case spine::BlendMode_Multiply: custom_material = slot_node->get_multiply_material(); break;
					case spine::BlendMode_Screen: custom_material = slot_node->get_screen_material(); break;
					}
				}
			}

			// Else, check if we have a material on the sprite itself
			if (!custom_material.is_valid()) {
				switch (blend_mode) {
				case spine::BlendMode_Normal: custom_material = normal_material; break;
				case spine::BlendMode_Additive: custom_material = additive_material; break;
				case spine::BlendMode_Multiply: custom_material = multiply_material; break;
				case spine::BlendMode_Screen: custom_material = screen_material; break;
				}
			}

			// Set the custom material, or the default material
			if (custom_material.is_valid()) mesh_instance->set_material(custom_material);
			else mesh_instance->set_material(default_materials[slot->getData().getBlendMode()]);
		}
		skeleton_clipper->clipEnd(*slot);
	}
	skeleton_clipper->clipEnd();
}

void SpineSprite::callback(spine::AnimationState *state, spine::EventType type, spine::TrackEntry *entry, spine::Event *event) {
	Ref<SpineTrackEntry> entry_ref = Ref<SpineTrackEntry>(memnew(SpineTrackEntry));
	entry_ref->set_spine_object(this,  entry);
	
	Ref<SpineEvent> event_ref(nullptr);
	if (event) {
		event_ref = Ref<SpineEvent>(memnew(SpineEvent));
		event_ref->set_spine_object(this, event);
	}

	switch (type) {
		case spine::EventType_Start:
			emit_signal("animation_started", this, animation_state, entry_ref);
			break;
		case spine::EventType_Interrupt:
			emit_signal("animation_interrupted", this, animation_state, entry_ref);
			break;
		case spine::EventType_End:
			emit_signal("animation_ended", this, animation_state, entry_ref);
			break;
		case spine::EventType_Complete:
			emit_signal("animation_completed", this, animation_state, entry_ref);
			break;
		case spine::EventType_Dispose:
			emit_signal("animation_disposed", this, animation_state, entry_ref);
			break;
		case spine::EventType_Event:
			emit_signal("animation_event", this, animation_state, entry_ref, event_ref);
			break;
	}
}

Transform2D SpineSprite::get_global_bone_transform(const String &bone_name) {
	if (!animation_state.is_valid() && !skeleton.is_valid()) return get_global_transform();
	auto bone = skeleton->find_bone(bone_name);
	if (!bone.is_valid()) {
		print_error(vformat("Bone: '%s' not found.", bone_name));
		return get_global_transform();
	}
	return bone->get_global_transform();
}

void SpineSprite::set_global_bone_transform(const String &bone_name, Transform2D transform) {
	if (!animation_state.is_valid() && !skeleton.is_valid()) return;
	auto bone = skeleton->find_bone(bone_name);
	if (!bone.is_valid()) return;
	bone->set_global_transform(transform);
}

SpineSprite::UpdateMode SpineSprite::get_update_mode() {
	return update_mode;
}

void SpineSprite::set_update_mode(SpineSprite::UpdateMode v) {
	update_mode = v;
	set_process_internal(update_mode == UpdateMode_Process);
	set_physics_process_internal(update_mode == UpdateMode_Physics);
}

Ref<SpineSkin> SpineSprite::new_skin(const String& name) {
	Ref<SpineSkin> skin = memnew(SpineSkin);
	skin->init(name, this);
	return skin;
}

Ref<Material> SpineSprite::get_normal_material() {
	return normal_material;
}

void SpineSprite::set_normal_material(Ref<Material> material) {
	normal_material = material;
}

Ref<Material> SpineSprite::get_additive_material() {
	return additive_material;
}

void SpineSprite::set_additive_material(Ref<Material> material) {
	additive_material = material;
}

Ref<Material> SpineSprite::get_multiply_material() {
	return multiply_material;
}

void SpineSprite::set_multiply_material(Ref<Material> material) {
	multiply_material = material;
}

Ref<Material> SpineSprite::get_screen_material() {
	return screen_material;
}

void SpineSprite::set_screen_material(Ref<Material> material) {
	screen_material = material;
}

#ifdef TOOLS_ENABLED
Rect2 SpineSprite::_edit_get_rect() const {
	if (skeleton_data_res.is_valid() && skeleton_data_res->is_skeleton_data_loaded()) {
		auto data = skeleton_data_res->get_skeleton_data();
		return Rect2(data->getX(), -data->getY() - data->getHeight(), data->getWidth(), data->getHeight());
	}

	return Node2D::_edit_get_rect();
}

bool SpineSprite::_edit_use_rect() const {
	return skeleton_data_res.is_valid() && skeleton_data_res->is_skeleton_data_loaded();
}
#endif
