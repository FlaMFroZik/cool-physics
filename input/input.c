/*
 * input.c — GLFW keyboard, mouse, and window callbacks.
 *
 * Interactive controls (work in both SIM_CONSTANT and SIM_NEWTON):
 *   - Left click on a ball + drag : move / throw the ball
 *   - Left click on empty space   : create a new ball (drag to position/throw)
 *   - Middle click or N / +       : spawn a new ball at the cursor
 *   - Right click (or drag)       : delete the ball under the cursor
 *   - Mouse wheel or , / . (< >)  : decrease / increase ball size
 *   - X / Delete / Backspace / -  : delete hovered ball (or last ball)
 *   - C                           : clear all balls
 */
#include "input/input.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

#include "render/camera.h"
#include "render/render.h"
#include "scene/scene.h"

static void update_cursor_world(App *app, double sx, double sy)
{
    int win_w = WINDOW_W, win_h = WINDOW_H;
    if (app->window)
        glfwGetWindowSize(app->window, &win_w, &win_h);
    app->cursor_world = camera_screen_to_world(&app->world, sx, sy, win_w, win_h);
    app->cursor_valid = true;
}

static void release_drag(App *app)
{
    World *w = &app->world;
    if (w->dragged_index >= 0 && (size_t)w->dragged_index < w->count)
        w->bodies[w->dragged_index].dragged = false;
    w->dragged_index = -1;
}

static void begin_drag(App *app, size_t idx)
{
    World *w = &app->world;
    if (idx >= w->count)
        return;
    release_drag(app);
    w->dragged_index       = (ptrdiff_t)idx;
    w->bodies[idx].dragged = true;
    w->bodies[idx].vel     = vec2_make(0, 0);
    app->drag_offset       = vec2_sub(w->bodies[idx].pos, app->cursor_world);
    app->drag_last_pos     = w->bodies[idx].pos;
}

static void on_error(int code, const char *description)
{
    fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

static void on_framebuffer_size(GLFWwindow *window, int w, int h)
{
    (void)window;
    render_resize(w, h);
}

static void on_cursor_pos(GLFWwindow *window, double xpos, double ypos)
{
    App *app = glfwGetWindowUserPointer(window);
    if (!app)
        return;

    update_cursor_world(app, xpos, ypos);
    World *w = &app->world;

    if (w->dragged_index >= 0 && (size_t)w->dragged_index < w->count) {
        Body *b   = &w->bodies[w->dragged_index];
        Vec2  pos = vec2_add(app->cursor_world, app->drag_offset);
        b->pos    = phys_world_clamp_pos(w, pos, b->radius);
    } else if (app->rmb_held) {
        scene_remove_ball_at(app, app->cursor_world);
    }

    w->hovered_index = phys_world_pick(w, app->cursor_world, (phys_real)0.5);
}

static void on_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
    (void)mods;
    App *app = glfwGetWindowUserPointer(window);
    if (!app)
        return;

    double sx = 0.0, sy = 0.0;
    glfwGetCursorPos(window, &sx, &sy);
    update_cursor_world(app, sx, sy);

    World *w = &app->world;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            ptrdiff_t idx = phys_world_pick(w, app->cursor_world, (phys_real)0.5);
            if (idx >= 0) {
                /* Grab existing ball to move it */
                begin_drag(app, (size_t)idx);
            } else {
                /* Click on empty space: create a new ball and grab it */
                Body *b = scene_spawn_ball(app, app->cursor_world,
                                           vec2_make(0, 0));
                if (b && w->count > 0)
                    begin_drag(app, w->count - 1);
            }
        } else if (action == GLFW_RELEASE) {
            release_drag(app);
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            app->rmb_held = true;
            scene_remove_ball_at(app, app->cursor_world);
        } else if (action == GLFW_RELEASE) {
            app->rmb_held = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        scene_spawn_ball(app, app->cursor_world, vec2_make(0, 0));
    }

    w->hovered_index = phys_world_pick(w, app->cursor_world, (phys_real)0.5);
}

static void on_scroll(GLFWwindow *window, double xoffset, double yoffset)
{
    (void)xoffset;
    App *app = glfwGetWindowUserPointer(window);
    if (!app || yoffset == 0.0)
        return;

    phys_real factor = (phys_real)pow(1.10, yoffset);
    scene_set_ball_radius(app, app->ball_radius * factor);
}

static void on_key(GLFWwindow *window, int key, int scancode, int action,
                   int mods)
{
    (void)scancode;
    (void)mods;
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
    App *app = glfwGetWindowUserPointer(window);
    if (!app)
        return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_SPACE:
        app->paused = !app->paused;
        break;
    case GLFW_KEY_R:
        rng_seed(&app->rng, (uint64_t)time(NULL));
        scene_spawn(app);
        break;
    case GLFW_KEY_N:
    case GLFW_KEY_EQUAL:
    case GLFW_KEY_KP_ADD:
        scene_spawn_ball_at_cursor(app);
        break;
    case GLFW_KEY_X:
    case GLFW_KEY_DELETE:
    case GLFW_KEY_BACKSPACE:
    case GLFW_KEY_MINUS:
    case GLFW_KEY_KP_SUBTRACT:
        if (!app->cursor_valid ||
            !scene_remove_ball_at(app, app->cursor_world)) {
            scene_remove_last_ball(app);
        }
        break;
    case GLFW_KEY_C:
        phys_world_clear(&app->world);
        break;
    case GLFW_KEY_COMMA:
    case GLFW_KEY_PAGE_DOWN:
        scene_set_ball_radius(app, app->ball_radius / (phys_real)1.10);
        break;
    case GLFW_KEY_PERIOD:
    case GLFW_KEY_PAGE_UP:
        scene_set_ball_radius(app, app->ball_radius * (phys_real)1.10);
        break;
    case GLFW_KEY_UP:
        app->time_scale *= (phys_real)1.25;
        if (app->time_scale > (phys_real)8)
            app->time_scale = (phys_real)8;
        break;
    case GLFW_KEY_DOWN:
        app->time_scale /= (phys_real)1.25;
        if (app->time_scale < (phys_real)0.0625)
            app->time_scale = (phys_real)0.0625;
        break;
    case GLFW_KEY_LEFT_BRACKET:
        if (app->subticks > 1)
            app_set_subticks(app, app->subticks - 1);
        break;
    case GLFW_KEY_RIGHT_BRACKET:
        if (app->subticks < MAX_SUBTICKS)
            app_set_subticks(app, app->subticks + 1);
        break;
    case GLFW_KEY_V:
        app->world.show_velocities = !app->world.show_velocities;
        break;
    case GLFW_KEY_B:
        app->world.show_grid = !app->world.show_grid;
        break;
    case GLFW_KEY_G:
        app->world.walls_enabled = !app->world.walls_enabled;
        break;
    default:
        break;
    }
}

void input_register_error_callback(void)
{
    glfwSetErrorCallback(on_error);
}

void input_init(App *app)
{
    glfwSetWindowUserPointer(app->window, app);
    glfwSetFramebufferSizeCallback(app->window, on_framebuffer_size);
    glfwSetKeyCallback(app->window, on_key);
    glfwSetMouseButtonCallback(app->window, on_mouse_button);
    glfwSetCursorPosCallback(app->window, on_cursor_pos);
    glfwSetScrollCallback(app->window, on_scroll);
}

void input_update_drag(App *app, double frame_dt)
{
    World *w = &app->world;
    if (w->dragged_index < 0 || (size_t)w->dragged_index >= w->count) {
        w->dragged_index = -1;
        return;
    }

    Body *b      = &w->bodies[w->dragged_index];
    Vec2  target = vec2_add(app->cursor_world, app->drag_offset);
    b->pos       = phys_world_clamp_pos(w, target, b->radius);

    if (frame_dt > 1e-5) {
        Vec2 raw_vel = vec2_scale(vec2_sub(b->pos, app->drag_last_pos),
                                  (phys_real)(1.0 / frame_dt));
        /* Clamp throw speed so fast mouse flicks stay stable. */
        static const phys_real MAX_THROW_SPEED = (phys_real)120;
        phys_real speed = vec2_len(raw_vel);
        if (speed > MAX_THROW_SPEED)
            raw_vel = vec2_scale(raw_vel, MAX_THROW_SPEED / speed);
        b->vel = vec2_add(vec2_scale(b->vel, (phys_real)0.4),
                          vec2_scale(raw_vel, (phys_real)0.6));
    }
    app->drag_last_pos = b->pos;
}
