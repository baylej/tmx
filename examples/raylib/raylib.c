/*
    TMX usage example with Raylib

    Declare callback functions like `tmx_img_load_func`
    in an OpenGL context i.e., after `InitWindow`.

    To enable support for .jpg and other image file formats,
    uncomment required in textures.c and rebuild Raylib.
    See "Development Platforms" section in Raylib wiki.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tmx.h"
#include "raylib.h"

#define fatal_error(str)  { fputs(str, stderr); goto errquit; }
#define DISPLAY_H 480
#define DISPLAY_W 640


Vector2 player = {0.0f, 0.0f}; /* invisible, just a dot: (x,y) */


struct Color set_color(unsigned int objgrColor)
{
    struct Color color;
    color.a = 255;  /* Alpha not implemented yet (Issue #39) */
    color.r = (objgrColor >> 16) & 0xFF;
    color.g = (objgrColor >>  8) & 0xFF;
    color.b = (objgrColor)       & 0xFF;
    return color;
}

void* raylib_load_image(const char *path)
{
    Texture2D texture = LoadTexture(path);
    Texture2D *returnValue = malloc(sizeof(Texture2D));
    memcpy(returnValue, &texture, sizeof(Texture2D));
    return returnValue;
}

void raylib_free_image(void *ptr)
{
    UnloadTexture(*((Texture2D*)ptr));
    free(ptr);
}

void draw_polyline(double **points, double x, double y, int pointsc, struct Color color)
{
    int i;
    for (i=1; i<pointsc; i++) {
        DrawLine(x+points[i-1][0], y+points[i-1][1], x+points[i][0], y+points[i][1], color);
    }
}

void draw_polygon(double **points, double x, double y, int pointsc, struct Color color)
{
    draw_polyline(points, x, y, pointsc, color);
    if (pointsc > 2) {
        DrawLine(x+points[0][0], y+points[0][1], x+points[pointsc-1][0], y+points[pointsc-1][1], color);
    }
}

void draw_objects(tmx_object_group *objgr)
{
    struct Color color = set_color(objgr->color);
    tmx_object *head = objgr->head;
    Rectangle rect;
    Vector2 *vectorPoints; /* can be fixed array to avoid malloc*/
    double **raw_points;
    int x, y, i;


    while (head) {
        if (head->visible) {
            if (head->obj_type == OT_SQUARE) {
                rect.x = head->x;
                rect.y = head->y;
                rect.width = head->width;
                rect.height = head->height;
                DrawRectangleLines(rect.x -player.x, rect.y -player.y, rect.width, rect.height, color);
            } else if (head->obj_type  == OT_POLYGON) {
                draw_polygon(head->content.shape->points, head->x -player.x, head->y -player.y, head->content.shape->points_len, color);
            } else if (head->obj_type == OT_POLYLINE) {

                /* You have to make Vector2* out of double** data points */
                vectorPoints = malloc (head->content.shape->points_len * sizeof (Vector2));
                for (i = 0; i < head->content.shape->points_len; i++) {
                    raw_points = head->content.shape->points;
                    x = head->x + raw_points[i][0];
                    y = head->y + raw_points[i][1];
                    vectorPoints->x = x -player.x;
                    vectorPoints->y = y -player.y;
                    vectorPoints++;
                }
                vectorPoints -= i;
                DrawPolyExLines(vectorPoints, head->content.shape->points_len, color);

                /* Or you can use this */
                //draw_polyline(head->content.shape->points, head->x -player.x, head->y -player.y, head->content.shape->points_len, color);
            } else if (head->obj_type == OT_TEXT) {
                DrawText(head->content.text->text, head->x -player.x, head->y -player.y, head->content.text->pixelsize, color);
            } else if (head->obj_type == OT_ELLIPSE) {
                /* Ellipse function */
            }
        }
        head = head->next;
    }
}

unsigned int gid_clear_flags(unsigned int gid)
{
    return gid & TMX_FLIP_BITS_REMOVAL;
}

void draw_layer(tmx_map *map, tmx_layer *layer)
{
    int i, j;
    unsigned int gid;
    tmx_tileset *ts;
    tmx_image *im;
    Rectangle srcrect;
    Vector2 dstrect;
    Texture* tileset;
    for (i=0; i<map->height; i++) {
        for (j=0; j<map->width; j++) {
            gid = gid_clear_flags(layer->content.gids[(i*map->width)+j]);
            if (map->tiles[gid] != NULL) {
                ts = map->tiles[gid]->tileset;
                im = map->tiles[gid]->image;
                srcrect.x = map->tiles[gid]->ul_x;
                srcrect.y = map->tiles[gid]->ul_y;
                srcrect.width = ts->tile_width;
                srcrect.height = ts->tile_height;
                dstrect.x = j*ts->tile_width;
                dstrect.y = i*ts->tile_height;
                  dstrect.x -= player.x;
                  dstrect.y -= player.y;
                if (im) {
                    tileset = (Texture*)im->resource_image;
                } else {
                    tileset = (Texture*)ts->image->resource_image;
                }
                DrawTextureRec(*tileset, srcrect, dstrect, WHITE);
            }
        }
    }
}

/*  Further optimization is up to you:
 *    Raylib function `LoadImagePro` could fit well to load
 *    an Image out of raw data (img->resource_image) but no way
 *    to get needed `format` (PixelFormat) argument using TMX.
 *  For example, load Textures before the main game loop
 *    and pass them though `render_map`.
 *  Left this implementation in order to keep the same structure
 *    as in SDL example.
 */
void draw_image_layer(tmx_image *img)
{
    Rectangle dim;
    dim.x = dim.y = 0;
    char path[PATH_MAX] = "data/";  /* ATTENTION: relative path */
    strcat(path, img->source);

    RenderTexture2D renTexture = LoadRenderTexture(DISPLAY_W, DISPLAY_H);
    BeginTextureMode(renTexture);
        Texture texture = LoadTexture(path);
        DrawTexture(texture, dim.x - player.x, dim.y - player.y, WHITE);
        UnloadRenderTexture(renTexture);
    EndTextureMode();
    UnloadTexture(texture);
}

void draw_all_layers(tmx_map *map, tmx_layer *layers)
{
    while (layers) {
        if (layers->visible) {
            if (layers->type == L_GROUP) {
                draw_all_layers(map, layers->content.group_head);
            } else if (layers->type == L_OBJGR) {
                draw_objects(layers->content.objgr);
            } else if (layers->type == L_IMAGE) {
                draw_image_layer(layers->content.image);
            } else if (layers->type == L_LAYER) {
                draw_layer(map, layers);
            }
        }
        layers = layers->next;
    }
}

Texture* render_map(tmx_map *map)
{
    Texture *res = NULL;
    draw_all_layers(map, map->ly_head);
    return res;
}

void update_movement(Vector2 *player, tmx_map *map)
{
    /* borders check */
    if (player->x <= 0) {
        if (IsKeyDown( KEY_A )) {
            return;
        }
    }
    if (player->x >= map->width * map->tile_width - DISPLAY_W) {
        if (IsKeyDown( KEY_D )) {
            return;
        }
    }
    if (player->y <= 0) {
        if (IsKeyDown( KEY_W )) {
            return;
        }
    }
    if (player->y >= map->height * map->tile_height - DISPLAY_H) {
        if (IsKeyDown( KEY_S )) {
            return;
        }
    }

    /* moves */
    if (IsKeyDown( KEY_W )) {
        player->y -= map->tile_height/2;
    }
    if (IsKeyDown( KEY_S )) {
        player->y += map->tile_height/2;
    }
    if (IsKeyDown( KEY_A )) {
        player->x -= map->tile_width/2;
    }
    if (IsKeyDown( KEY_D )) {
        player->x += map->tile_width/2;
    }
}


int main()
{
    InitWindow(DISPLAY_W, DISPLAY_H, "raylib TMX");
    SetTargetFPS(60);

    tmx_img_load_func = (void* (*)( const char* )) raylib_load_image;
    tmx_img_free_func = (void  (*)( void* )) raylib_free_image;

    tmx_map *map = NULL;
    if (!(map = tmx_load("data/objecttemplates.tmx")))
        fatal_error(tmx_strerr());

    /* Main game loop */
    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            render_map(map);
            update_movement(&player, map); /* map: for both borders and speed */
        EndDrawing();
    }

    tmx_map_free(map);
    CloseWindow();
    return 0;

errquit:
    CloseWindow();
    return -1;
}
