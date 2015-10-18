
/*

		Copyright 2015 Zachary Moore

		This file is part of friendsfind.me

		FriendsFind is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

		FriendsFind is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

		You should have received a copy of the GNU General Public License along with Foobar. If not, see http://www.gnu.org/licenses/.
*/

#include <pebble.h>

Window *window;

GPath* arrow;
CompassHeading north;

AppSync sync;
static uint8_t sync_buffer[32];
#define KEY_COUNT 0


#define MAX_FRIENDS 5
int nFriends = 0;
char** friendNames = (char*[]){"Alice","Bob","Carol","Dave","Eduardo"};
CompassHeading* friendDirections = (CompassHeading[]){0,TRIG_MAX_ANGLE / 5, TRIG_MAX_ANGLE *2/5, TRIG_MAX_ANGLE*3/5, TRIG_MAX_ANGLE*4/5};
uint8_t* friendDistances = (uint8_t[]){10,20,30,40,50};
#define friendColors ((GColor[]){GColorRed, GColorBlue, GColorYellow, GColorGreen, GColorMagenta})

void get_data(DictionaryIterator *iterator, void *context) {
	Tuple *tuple = dict_read_first(iterator);
	while (tuple) {
		if (tuple -> key) {
			friendNames[tuple->key-1] = tuple->value->cstring;
		} else {
			nFriends = tuple->length;
			for (int i = 0; i < nFriends; i++)
				friendDirections[i] = DEG_TO_TRIGANGLE(tuple->value->data[i]);
		}
		tuple = dict_read_next(iterator);
	}
	layer_mark_dirty(window_get_root_layer(window));
}

void compass_handler(CompassHeadingData heading){
	north = heading.true_heading;
	// Let arrow redraw itself
	layer_mark_dirty(window_get_root_layer(window));
}

void textOutline(GContext *ctx, const char *text, const GFont font, const GRect box, const GTextOverflowMode overflow_mode, const GTextAlignment alignment, GTextAttributes *text_attributes, const GColor fg, const GColor bg){
	int w = box.size.w;
	int h = box.size.h;
	int x = box.origin.x;
	int y = box.origin.y;
	graphics_context_set_text_color(ctx, bg);
	graphics_draw_text(ctx, text, font, GRect(x+1,y,w,h), overflow_mode, alignment, text_attributes);
	graphics_draw_text(ctx, text, font, GRect(x-1,y,w,h), overflow_mode, alignment, text_attributes);
	graphics_draw_text(ctx, text, font, GRect(x,y+1,w,h), overflow_mode, alignment, text_attributes);
	graphics_draw_text(ctx, text, font, GRect(x,y-1,w,h), overflow_mode, alignment, text_attributes);
	
	graphics_context_set_text_color(ctx, fg);
	graphics_draw_text(ctx, text, font, box, overflow_mode, alignment, text_attributes);
}

void draw_arrow(Layer *this_layer, GContext *ctx){
	GRect bounds = layer_get_bounds(this_layer);
	GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
	int i;
	int size;
	GRect area;
	arrow -> offset = center;
	if (nFriends == 0){
		area = GRect(center.x-50,center.y-20,100,40);
		graphics_context_set_fill_color(ctx, GColorRed);
		graphics_fill_rect(ctx, area, 5, GCornersAll);
		graphics_context_set_text_color(ctx, GColorBlack);
		graphics_draw_text(ctx,
			"No nearby friends :-(",
			fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
			area,
			GTextOverflowModeFill,
			GTextAlignmentCenter,
			NULL);
		return;
	}
	for (i=0; i < nFriends; i++){
		//Arrow setup
		size = 40;//friendDistances[i];
		arrow -> points = (GPoint []) {
			{0, 0},
			{size/2, -size},
			{size, -size},
			{0, -2*size},
			{-size, -size},
			{-size/2, -size},
			{0, 0}
		};
		//Draw arrow
		arrow -> rotation = friendDirections[i] + north;
		graphics_context_set_fill_color(ctx, friendColors[i]);
		gpath_draw_filled(ctx, arrow);
		//Draw name
		area = GRect(
			center.x-25+sin_lookup(arrow->rotation)*(50) / TRIG_MAX_RATIO,
			center.y-7-cos_lookup(arrow->rotation)*(50) / TRIG_MAX_RATIO,
			50,
			15);
		textOutline(ctx,
			friendNames[i],
			fonts_get_system_font(FONT_KEY_GOTHIC_14),
			area,
			GTextOverflowModeTrailingEllipsis,
			GTextAlignmentCenter,
			NULL,
			friendColors[i],
			GColorBlack);
	}
}

void handle_init(void) {
	GPathInfo arrowPath = {
		.num_points = 7,
		.points = NULL
	};
	// Create the arrow
	arrow = gpath_create(&arrowPath);
	
	// Create a window
	window = window_create();
	
	// Set arrow redraw function
	layer_set_update_proc(window_get_root_layer(window), draw_arrow);
	
	// Push the window
	window_stack_push(window, true);
	
	// Start sync with phone	
	app_message_open(app_message_inbox_size_maximum(), 0 /*Nothing outgoing*/);
	app_message_register_inbox_received(get_data);

	// Listen to compass
	compass_service_set_heading_filter(0);
	compass_service_subscribe(compass_handler);

	vibes_double_pulse();
}

void handle_deinit(void) {
	// Destroy the window
	window_destroy(window);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
