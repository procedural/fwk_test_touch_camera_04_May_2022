// original code by @vurtun (public domain)
// - rlyeh, public domain.
//
// @todo: fix leaks: poly_free()

#include "fwk.h"

int paused;
camera_t cam;

int  gCanvasWidth;
int  gCanvasHeight;
long gx0;
long gy0;
long gx1;
long gy1;
long gStartMoveX;
long gStartMoveY;
long gPrevTouchX0;
long gPrevTouchY0;
long gPrevTouchX1;
long gPrevTouchY1;
int  gSkipTouch;

void game_loop(void *userdata) {
    if(!window_swap()) return;

    // NOTE(Constantine): Touch events for every frame.
    {
      const int canvasHalfWidth = gCanvasWidth / 2;

      const float sensitivity = 0.005f;
      long moveX      = 0;
      long moveY      = 0;
      long startMoveX = 0;
      long startMoveY = 0;
      if (gStartMoveX != 0 && gStartMoveY != 0) {
        if (gx0 > 0 && gx0 < canvasHalfWidth) {
          moveX      = gx0;
          moveY      = gy0;
          startMoveX = gStartMoveX;
          startMoveY = gStartMoveY;
        } else if (gx1 > 0 && gx1 < canvasHalfWidth) {
          moveX      = gx1;
          moveY      = gy1;
          startMoveX = gStartMoveX;
          startMoveY = gStartMoveY;
        }
      }
      camera_move(&cam, (moveX - startMoveX) * sensitivity, 0, (startMoveY - moveY) * sensitivity);
    }

    // key handler
    if (input_down(KEY_F11) ) window_fullscreen( window_has_fullscreen()^1 );
    if (input_down(KEY_ESC) ) window_loop_exit(); // @todo: break -> window_close()

    // animation
    static float dx = 0, dy = 0;
    if (input_down(KEY_SPACE)) paused ^= 1;
    float delta = (0.25f / 60.f) * !paused;
    dx = dx + delta * 2.0f;
    dy = dy + delta * 0.8f;

    // fps camera
    bool active = ui_active() || ui_hover() || gizmo_active() ? false : input(MOUSE_L) || input(MOUSE_M) || input(MOUSE_R);
    window_cursor( !active );

    if( active ) cam.speed = clampf(cam.speed + input_diff(MOUSE_W) / 10, 0.05f, 5.0f);
    vec2 mouse = scale2(vec2(input_diff(MOUSE_X), -input_diff(MOUSE_Y)), 0.2f * active);
    vec3 wasdec = scale3(vec3(input(KEY_D)-input(KEY_A),input(KEY_E)-input(KEY_C),input(KEY_W)-input(KEY_S)), cam.speed);
    camera_move(&cam, wasdec.x,wasdec.y,wasdec.z);
    camera_fps(&cam, mouse.x,mouse.y);

    // projview matrix
    mat44 projview; multiply44x2(projview, cam.proj, cam.view);

    // rendering
    viewport_color3(vec3(0.15,0.15,0.15));
    viewport_clear(true, true);
    viewport_clip(vec2(0,0), vec2(window_width(), window_height()));

    // debug draw collisions
    {
        // 3D
        glEnable(GL_DEPTH_TEST);

        // grid
        ddraw_grid(0);

        {
            // Triangle-Ray Intersection*/
            vec3 ro, rd;
            int suc;

            triangle tri = { vec3(-9,1,28), vec3(-10,0,28), vec3(-11,1,28) };

            // ray
            ro = vec3(-10,-1,20);
            rd = vec3(-10+0.4f*sinf(dx), 2.0f*cosf(dy), 29.81023f);
            rd = sub3(rd, ro);
            rd = norm3(rd);

            ray r = ray(ro, rd);
            hit *hit = ray_hit_triangle(r, tri);
            if (hit) {
                // point of intersection
                ddraw_color(RED);
                ddraw_box(hit->p, vec3(0.10f, 0.10f, 0.10f));

                // intersection normal
                ddraw_color(BLUE);
                vec3 v = add3(hit->p, hit->n);
                ddraw_arrow(hit->p, v);
            }

            // line
            ddraw_color(RED);
            rd = scale3(rd,10);
            rd = add3(ro,rd);
            ddraw_line(ro, rd);

            // triangle
            if (hit) ddraw_color(RED);
            else ddraw_color(WHITE);
            ddraw_triangle(tri.p0,tri.p1,tri.p2);
        }
        {
            // Plane-Ray Intersection*/
            vec3 ro, rd;
            mat33 rot;

            // ray
            static float d = 0;
            d += delta * 2.0f;
            ro = vec3(0,-1,20);
            rd = vec3(0.1f, 0.5f, 9.81023f);
            rd = sub3(rd, ro);
            rd = norm3(rd);

            // rotation
            rotation33(rot, deg(d), 0,1,0);
            rd = mulv33(rot, rd);

            // intersection
            ray r = ray(ro, rd);
            plane pl = plane(vec3(0,0,28), vec3(0,0,1));
            hit *hit = ray_hit_plane(r, pl);
            if (hit) {
                // point of intersection
                ddraw_color(RED);
                ddraw_box(hit->p, vec3(0.10f, 0.10f, 0.10f));

                // intersection normal
                ddraw_color(BLUE);
                vec3 v = add3(hit->p, hit->n);
                ddraw_arrow(hit->p, v);
                ddraw_color(RED);
            }
            // line
            ddraw_color(RED);
            rd = scale3(rd,9);
            rd = add3(ro,rd);
            ddraw_line(ro, rd);

            // plane
            if (hit) ddraw_color(RED);
            else ddraw_color(WHITE);
            ddraw_plane(vec3(0,0,28), vec3(0,0,1), 3.0f);
        }
        {
            // Sphere-Ray Intersection*/
            vec3 ro, rd;
            sphere s;

            // ray
            ro = vec3(0,-1,0);
            rd = vec3(0.4f*sinf(dx), 2.0f*cosf(dy), 9.81023f);
            rd = sub3(rd, ro);
            rd = norm3(rd);

            ray r = ray(ro, rd);
            s = sphere(vec3(0,0,8), 1);
            hit *hit = ray_hit_sphere(r, s);
            if(hit) {
                // points of intersection
                vec3 in = add3(ro,scale3(rd,hit->t0));

                ddraw_color(GREEN);
                ddraw_box(in, vec3(0.05f, 0.05f, 0.05f));

                in = add3(ro,scale3(rd,hit->t1));

                ddraw_color(YELLOW);
                ddraw_box(in, vec3(0.05f, 0.05f, 0.05f));

                // intersection normal
                ddraw_color(BLUE);
                vec3 v = add3(hit->p, hit->n);
                ddraw_arrow(hit->p, v);
                ddraw_color(RED);
            }
            // line
            ddraw_color(RED);
            rd = scale3(rd,10);
            rd = add3(ro,rd);
            ddraw_line(ro, rd);

            // sphere
            if (hit) ddraw_color(RED);
            else ddraw_color(WHITE);
            ddraw_sphere(vec3(0,0,8), 1);
        }
        {   // ray-aabb
            aabb bounds = aabb(vec3(10-0.5f,-0.5f,7.5f), vec3(10.5f,0.5f,8.5f));

            vec3 ro = vec3(10,-1,0);
            vec3 rd = vec3(10+0.4f*sinf(dx), 2.0f*cosf(dy), 9.81023f);
            rd = norm3(sub3(rd, ro));
            ray r = ray(ro, rd);

            hit *hit = ray_hit_aabb(r, bounds);
            if(hit) {
                // points of intersection
                vec3 in;
                in = scale3(rd,hit->t0);
                in = add3(ro,in);

                ddraw_color(RED);
                ddraw_box(in, vec3(0.05f, 0.05f, 0.05f));

                in = scale3(rd,hit->t1);
                in = add3(ro,in);

                ddraw_color(RED);
                ddraw_box(in, vec3(0.05f, 0.05f, 0.05f));

                // intersection normal
                ddraw_color(BLUE);
                vec3 v = add3(hit->p, hit->n);
                ddraw_arrow(hit->p, v);
                ddraw_color(RED);
            } else ddraw_color(WHITE);
            ddraw_box(vec3(10,0,8), vec3(1,1,1));

            // line
            ddraw_color(RED);
            rd = scale3(rd,10);
            rd = add3(ro,rd);
            ddraw_line(ro, rd);
        }
        {
            // Sphere-Sphere intersection*/
            sphere a = sphere(vec3(-10,0,8), 1);
            sphere b = sphere(vec3(-10+0.6f*sinf(dx), 3.0f*cosf(dy),8), 1);
            hit *m = sphere_hit_sphere(a, b);
            if (m) {
                vec3 v;
                ddraw_color(BLUE);
                ddraw_box(m->contact_point, vec3(0.05f, 0.05f, 0.05f));
                v = add3(m->contact_point, m->normal);
                ddraw_arrow(m->contact_point, v);
                ddraw_color(RED);
            } else ddraw_color(WHITE);

            ddraw_sphere(a.c, 1);
            ddraw_sphere(b.c, 1);
        }
        {
            // AABB-AABB intersection*/
            const float x = 10+0.6f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = 20.0f;

            aabb a = aabb(vec3(10-0.5f,-0.5f,20-0.5f), vec3(10+0.5f,0.5f,20.5f));
            aabb b = aabb(vec3(x-0.5f,y-0.5f,z-0.5f), vec3(x+0.5f,y+0.5f,z+0.5f));
            hit *m = aabb_hit_aabb(a, b);
            if(m) {
                vec3 v;
                ddraw_color(BLUE);
                ddraw_box(m->contact_point, vec3(0.05f, 0.05f, 0.05f));
                v = add3(m->contact_point, m->normal);
                ddraw_arrow(m->contact_point, v);
                ddraw_color(RED);
            } else ddraw_color(WHITE);

            ddraw_box(vec3(10,0,20), vec3(1,1,1));
            ddraw_box(vec3(x,y,z), vec3(1,1,1));
        }
        {
            // Capsule-Capsule intersection*/
            const float x = 20+0.4f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = 28.5f;

            capsule a = capsule(vec3(20.0f,-1.0f,28.0f), vec3(20.0f,1.0f,28.0f), 0.2f);
            capsule b = capsule(vec3(x,y-1.0f,z), vec3(x,y+1.0f,z-1.0f), 0.2f);
            hit *m = capsule_hit_capsule(a, b);
            if( m ) {
                vec3 v;
                ddraw_color(BLUE);
                ddraw_box(m->contact_point, vec3(0.05f, 0.05f, 0.05f));
                v = add3(m->contact_point, m->normal);
                ddraw_arrow(m->contact_point, v);
                ddraw_color(RED);
            } else ddraw_color(WHITE);
            ddraw_capsule(vec3(x,y-1.0f,z), vec3(x,y+1.0f,z-1.0f), 0.2f);
            ddraw_capsule(vec3(20.0f,-1.0f,28.0f), vec3(20.0f,1.0f,28.0f), 0.2f);
        }
        {
            // AABB-Sphere intersection*/
            aabb a = aabb(vec3(20-0.5f,-0.5f,7.5f), vec3(20.5f,0.5f,8.5f));
            sphere s = sphere(vec3(20+0.6f*sinf(dx), 3.0f*cosf(dy),8), 1);
            hit *m  = aabb_hit_sphere(a, s);
            if(m) {
                vec3 v;
                ddraw_color(BLUE);
                ddraw_box(m->contact_point, vec3(0.05f, 0.05f, 0.05f));
                v = add3(m->contact_point, m->normal);
                ddraw_arrow(m->contact_point, v);
                ddraw_color(RED);
            } else ddraw_color(WHITE);

            ddraw_box(vec3(20,0,8), vec3(1,1,1));
            ddraw_sphere(s.c, 1);
        }
        {
            // Sphere-AABB intersection*/
            const float x = 10+0.6f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = -8.0f;

            sphere s = sphere(vec3(10,0,-8), 1);
            aabb a = aabb(vec3(x-0.5f,y-0.5f,z-0.5f), vec3(x+0.5f,y+0.5f,z+0.5f));
            hit *m = sphere_hit_aabb(s, a);
            if(m) {
                vec3 v;
                ddraw_color(BLUE);
                ddraw_box(m->contact_point, vec3(0.05f, 0.05f, 0.05f));
                v = add3(m->contact_point, m->normal);
                ddraw_arrow(m->contact_point, v);
                ddraw_color(RED);
            } else ddraw_color(WHITE);

            ddraw_box(vec3(x,y,z), vec3(1,1,1));
            ddraw_sphere(s.c, 1);
        }
        {
            // Capsule-Sphere intersection*/
            capsule c = capsule(vec3(-20.5f,-1.0f,7.5f), vec3(-20+0.5f,1.0f,8.5f), 0.2f);
            sphere b = sphere(vec3(-20+0.6f*sinf(dx), 3.0f*cosf(dy),8), 1);
            hit *m = capsule_hit_sphere(c, b);
            if(m) {
                vec3 v;
                ddraw_color(BLUE);
                ddraw_box(m->contact_point, vec3(0.05f, 0.05f, 0.05f));
                v = add3(m->contact_point, m->normal);
                ddraw_arrow(m->contact_point, v);
                ddraw_color(RED);
            } else ddraw_color(WHITE);
            ddraw_sphere(b.c, 1);
            ddraw_capsule(vec3(-20.5f,-1.0f,7.5f), vec3(-20+0.5f,1.0f,8.5f), 0.2f);
        }
        {
            // Sphere-Capsule intersection*/
            const float x = 20+0.4f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = -8;

            sphere s = sphere(vec3(20,0,-8), 1);
            capsule c = capsule(vec3(x,y-1.0f,z), vec3(x,y+1.0f,z-1.0f), 0.2f);
            hit *m = sphere_hit_capsule(s, c);
            if(m) {
                vec3 v;
                ddraw_color(BLUE);
                ddraw_box(m->contact_point, vec3(0.05f, 0.05f, 0.05f));
                v = add3(m->contact_point, m->normal);
                ddraw_arrow(m->contact_point, v);
                ddraw_color(RED);
            } else ddraw_color(WHITE);

            ddraw_capsule(vec3(x,y-1.0f,z), vec3(x,y+1.0f,z-1.0f), 0.2f);
            ddraw_sphere(s.c, 1);
        }
        {
            // Capsule-AABB intersection*/
            const float x = -20+0.6f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = 28.0f;

            capsule c = capsule(vec3(-20.5f,-1.0f,27.5f), vec3(-20+0.5f,1.0f,28.5f), 0.2f);
            aabb b = aabb(vec3(x-0.5f,y-0.5f,z-0.5f), vec3(x+0.5f,y+0.5f,z+0.5f));
            hit *m = capsule_hit_aabb(c, b);
            if(m) {
                vec3 v;
                ddraw_color(BLUE);
                ddraw_box(m->contact_point, vec3(0.05f, 0.05f, 0.05f));
                v = add3(m->contact_point, m->normal);
                ddraw_arrow(m->contact_point, v);
                ddraw_color(RED);
            } else ddraw_color(WHITE);
            ddraw_box(vec3(x,y,z), vec3(1,1,1));
            ddraw_capsule(vec3(-20.5f,-1.0f,27.5f), vec3(-20+0.5f,1.0f,28.5f), 0.2f);
        }
        {
            // AABB-Capsule intersection*/
            const float x = 0.4f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = -8;

            aabb a = aabb(vec3(-0.5f,-0.5f,-8.5f), vec3(0.5f,0.5f,-7.5f));
            capsule c = capsule(vec3(x,y-1.0f,z), vec3(x,y+1.0f,z-1.0f), 0.2f);
            hit *m = aabb_hit_capsule(a, c);
            if(m) {
                ddraw_color(RED);
                ddraw_box(m->contact_point, vec3(0.05f, 0.05f, 0.05f));
                ddraw_arrow(m->contact_point, add3(m->contact_point, m->normal));
            } else ddraw_color(WHITE);

            ddraw_capsule(vec3(x,y-1.0f,z), vec3(x,y+1.0f,z-1.0f), 0.2f);
            ddraw_box(vec3(0,0,-8.0f), vec3(1,1,1));
        }
        {
            // poly(Pyramid)-Sphere (GJK) intersection*/
            sphere s = sphere(vec3(-10+0.6f*sinf(dx), 3.0f*cosf(dy),-8), 1);
            poly pyr = pyramid(vec3(-10.5f,-0.5f,-7.5f), vec3(-10.5f,1.0f,-7.5f), 1.0f);

            gjk_result gjk;
            if (poly_hit_sphere(&gjk, pyr, s))
                ddraw_color(RED);
            else ddraw_color(WHITE);

            ddraw_sphere(s.c, 1);
            ddraw_pyramid(vec3(-10.5f,-0.5f,-7.5f), 0.5f/*vec3(-10.5f,1.0f,-7.5f)*/, 1.0f);

            poly_free(&pyr);

            ddraw_box(gjk.p0, vec3(0.05f, 0.05f, 0.05f));
            ddraw_box(gjk.p1, vec3(0.05f, 0.05f, 0.05f));
            ddraw_line(gjk.p0, gjk.p1);
        }
        {
            // poly(Diamond)-Sphere (GJK) intersection*/

            sphere s = sphere(vec3(-20+0.6f*sinf(dx), 3.0f*cosf(dy),-8), 1);
            poly dmd = diamond(vec3(-20.5f,-0.5f,-7.5f), vec3(-20.5f,1.0f,-7.5f), 0.5f);

            gjk_result gjk;
            if (poly_hit_sphere(&gjk, dmd, s))
                ddraw_color(RED);
            else ddraw_color(WHITE);

            ddraw_sphere(s.c, 1);
            ddraw_diamond(vec3(-20.5f,-0.5f,-7.5f), vec3(-20.5f,1.0f,-7.5f), 0.5f);

            poly_free(&dmd);

            ddraw_box(gjk.p0, vec3(0.05f, 0.05f, 0.05f));
            ddraw_box(gjk.p1, vec3(0.05f, 0.05f, 0.05f));
            ddraw_line(gjk.p0, gjk.p1);
        }
        {
            // poly(Pyramid)-Capsule (GJK) intersection*/

            const float x = 0.4f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = -15;

            capsule c = capsule(vec3(x,y-1.0f,z), vec3(x,y+1.0f,z), 0.2f);
            poly pyr = pyramid(vec3(-0.5f,-0.5f,-15.5f), vec3(-0.5f,1.0f,-15.5f), 1.0f);

            gjk_result gjk;
            if (poly_hit_capsule(&gjk, pyr, c))
                ddraw_color(RED);
            else ddraw_color(WHITE);

            ddraw_capsule(c.a, c.b, c.r);
            ddraw_pyramid(vec3(-0.5f,-0.5f,-15.5f), 0.5f/*vec3(-0.5f,1.0f,-15.5f)*/, 1.0f);

            poly_free(&pyr);

            ddraw_box(gjk.p0, vec3(0.05f, 0.05f, 0.05f));
            ddraw_box(gjk.p1, vec3(0.05f, 0.05f, 0.05f));
            ddraw_line(gjk.p0, gjk.p1);
        }

        {
            // poly(Diamond)-Capsule (GJK) intersection*/

            const float x = -10 + 0.4f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = -15;

            capsule c = capsule(vec3(x,y-1.0f,z), vec3(x,y+1.0f,z), 0.2f);
            poly dmd = diamond(vec3(-10.5f,-0.5f,-15.5f), vec3(-10.5f,1.0f,-15.5f), 0.5f);

            gjk_result gjk;
            if (poly_hit_capsule(&gjk, dmd, c))
                ddraw_color(RED);
            else ddraw_color(WHITE);

            ddraw_capsule(c.a, c.b, c.r);
            ddraw_diamond(vec3(-10.5f,-0.5f,-15.5f), vec3(-10.5f,1.0f,-15.5f), 0.5f);

            poly_free(&dmd);

            ddraw_box(gjk.p0, vec3(0.05f, 0.05f, 0.05f));
            ddraw_box(gjk.p1, vec3(0.05f, 0.05f, 0.05f));
            ddraw_line(gjk.p0, gjk.p1);
        }

        {
            // poly(Diamond)-poly(Pyramid) (GJK) intersection*/

            const float x = -20 + 0.4f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = -15;

            poly pyr = pyramid(vec3(x,y-0.5f,z), vec3(x,y+1,z), 0.8f);
            poly dmd = diamond(vec3(-20.5f,-0.5f,-15.5f), vec3(-20.5f,1.0f,-15.5f), 0.5f);

            gjk_result gjk;
            if (poly_hit_poly(&gjk, dmd, pyr))
                ddraw_color(RED);
            else ddraw_color(WHITE);

            ddraw_pyramid(vec3(x,y-0.5f,z), 1/*vec3(x,y+1,z)*/, 1/*0.8f*/);
            ddraw_diamond(vec3(-20.5f,-0.5f,-15.5f), vec3(-20.5f,1.0f,-15.5f), 0.5f);

            poly_free(&dmd);
            poly_free(&pyr);

            ddraw_box(gjk.p0, vec3(0.05f, 0.05f, 0.05f));
            ddraw_box(gjk.p1, vec3(0.05f, 0.05f, 0.05f));
            ddraw_line(gjk.p0, gjk.p1);
        }
        {
            // poly(Pyramid)-poly(Diamond) (GJK) intersection*/

            const float x = 10 + 0.4f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = -15;

            poly dmd = diamond(vec3(x,y-0.5f,z), vec3(x,y+1,z), 0.5f);
            poly pyr = pyramid(vec3(10.5f,-0.5f,-15.5f), vec3(10.5f,1.0f,-15.5f), 1.0f);

            gjk_result gjk;
            if (poly_hit_poly(&gjk, dmd, pyr))
                ddraw_color(RED);
            else ddraw_color(WHITE);

            ddraw_diamond(vec3(x,y-0.5f,z), vec3(x,y+1,z), 0.5f);
            ddraw_pyramid(vec3(10.5f,-0.5f,-15.5f), 0.5f/*vec3(10.5f,1.0f,-15.5f)*/, 1.0f);

            poly_free(&dmd);
            poly_free(&pyr);

            ddraw_box(gjk.p0, vec3(0.05f, 0.05f, 0.05f));
            ddraw_box(gjk.p1, vec3(0.05f, 0.05f, 0.05f));
            ddraw_line(gjk.p0, gjk.p1);
        }
        {
            // poly(Diamond)-AABB (GJK) intersection*/

            const float x = 20 + 0.4f*sinf(dx);
            const float y = 3.0f*cosf(dy);
            const float z = -15;

            poly dmd = diamond(vec3(x,y-0.5f,z), vec3(x,y+1,z), 0.5f);
            aabb a = aabb(vec3(19.5f,-0.5f,-14.5f), vec3(20.5f,0.5f,-15.5f));

            gjk_result gjk;
            if (poly_hit_aabb(&gjk, dmd, a))
                ddraw_color(RED);
            else ddraw_color(WHITE);

            poly_free(&dmd);

            ddraw_diamond(vec3(x,y-0.5f,z), vec3(x,y+1,z), 0.5f);
            ddraw_box(vec3(20,0,-15), vec3(1,1,1));

            ddraw_box(gjk.p0, vec3(0.05f, 0.05f, 0.05f));
            ddraw_box(gjk.p1, vec3(0.05f, 0.05f, 0.05f));
            ddraw_line(gjk.p0, gjk.p1);
        }
    }

    //fx_begin();
    ddraw_flush();
    //fx_end();

    if( ui_begin("UI", 0) ) {
        ui_label("test label");
        ui_end();
    }

    if( ui_begin("FX", 0) ) {
        for( int i = 0; i < 64; ++i ) {
            char *name = fx_name(i); if( !name ) break;
            bool b = fx_enabled(i);
            if( ui_bool(name, &b) ) fx_enable(i, fx_enabled(i) ^ 1);
        }
        ui_end();
    }
}

EM_BOOL touch_start(int eventType, const EmscriptenTouchEvent * e, void * userData) {
  gx0 = e->touches[0].screenX;
  gy0 = e->touches[0].screenY;
  gx1 = e->touches[1].screenX;
  gy1 = e->touches[1].screenY;

  const int canvasHalfWidth = gCanvasWidth / 2;

  if (gStartMoveX == 0 && gStartMoveY == 0) {
    if (gx0 > 0 && gx0 < canvasHalfWidth) {
      gStartMoveX = gx0;
      gStartMoveY = gy0;
    } else if (gx1 > 0 && gx1 < canvasHalfWidth) {
      gStartMoveX = gx1;
      gStartMoveY = gy1;
    }
  }

  gPrevTouchX0 = gx0;
  gPrevTouchY0 = gy0;
  gPrevTouchX1 = gx1;
  gPrevTouchY1 = gy1;

  return EM_TRUE;
}

EM_BOOL touch_end(int eventType, const EmscriptenTouchEvent * e, void * userData) {
  gx0 = e->touches[0].screenX;
  gy0 = e->touches[0].screenY;
  gx1 = e->touches[1].screenX;
  gy1 = e->touches[1].screenY;

  gSkipTouch = 1;

  if (e->numTouches == 1) {
    gStartMoveX = 0;
    gStartMoveY = 0;
  }

  gPrevTouchX0 = gx0;
  gPrevTouchY0 = gy0;
  gPrevTouchX1 = gx1;
  gPrevTouchY1 = gy1;

  return EM_TRUE;
}

EM_BOOL touch_move(int eventType, const EmscriptenTouchEvent * e, void * userData) {
  gx0 = e->touches[0].screenX;
  gy0 = e->touches[0].screenY;
  gx1 = e->touches[1].screenX;
  gy1 = e->touches[1].screenY;

  if (gSkipTouch == 1) {
    gSkipTouch = 0;

    gPrevTouchX0 = gx0;
    gPrevTouchY0 = gy0;
    gPrevTouchX1 = gx1;
    gPrevTouchY1 = gy1;

    return EM_TRUE;
  }

  const int canvasHalfWidth = gCanvasWidth / 2;

  const float sensitivity = 0.25f;
  long rotX     = 0;
  long rotY     = 0;
  long prevRotX = 0;
  long prevRotY = 0;
  if (gx0 > canvasHalfWidth) {
    rotX     = gx0;
    rotY     = gy0;
    prevRotX = gPrevTouchX0;
    prevRotY = gPrevTouchY0;
  } else if (gx1 > canvasHalfWidth) {
    rotX     = gx1;
    rotY     = gy1;
    prevRotX = gPrevTouchX1;
    prevRotY = gPrevTouchY1;
  }
  camera_fps(&cam, (rotX - prevRotX) * sensitivity, (prevRotY - rotY) * sensitivity);

  gPrevTouchX0 = gx0;
  gPrevTouchY0 = gy0;
  gPrevTouchX1 = gx1;
  gPrevTouchY1 = gy1;

  return EM_TRUE;
}

int main(void) {
    // 75% sized, msaa x4 enabled
    window_create(0.75f, WINDOW_MSAA4);
    window_title( "FWK - SPACE pauses simulation" );

    for(const char **list = vfs_list("fx**.fs"); *list; list++) {
        //fx_load(*list);
    }

    // camera that points to origin
    cam = camera();

    gx0          = 0;
    gy0          = 0;
    gx1          = 0;
    gy1          = 0;
    gStartMoveX  = 0;
    gStartMoveY  = 0;
    gPrevTouchX0 = 0;
    gPrevTouchY0 = 0;
    gPrevTouchX1 = 0;
    gPrevTouchY1 = 0;
    gSkipTouch   = 0;
    emscripten_get_canvas_element_size("#canvas", &gCanvasWidth, &gCanvasHeight);
    emscripten_set_touchstart_callback("#canvas", 0, EM_FALSE, &touch_start);
    emscripten_set_touchend_callback("#canvas", 0, EM_FALSE, &touch_end);
    emscripten_set_touchmove_callback("#canvas", 0, EM_FALSE, &touch_move);

    // main loop
    window_loop(game_loop, NULL);
}
