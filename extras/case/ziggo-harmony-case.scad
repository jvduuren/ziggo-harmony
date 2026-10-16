// ziggo-harmony case: Waveshare ESP32-S3-Zero + 3.5 mm panel jack for an external IR eye.
//
// Base with the board lying flat (USB-C through one end wall, jack through the other) and a
// snap-on lid with a light pipe above the board LED and a pin hole for BOOT.
// Print base and lid in any colour, the light pipe in transparent filament.
//
// Render one part:  openscad -D 'part="base"' -o base.stl ziggo-harmony-case.scad
// part = "base" | "lid" | "pipe" | "all" (preview, parts side by side)
part = "all";

/* [Board: Waveshare ESP32-S3-Zero] */
board_l = 23.7;        // along the USB axis
board_w = 18.0;
board_t = 1.7;
comp_h = 7.0;          // tallest part on top of the board, USB-C receptacle excluded
under_h = 3.0;         // solder joints / wires under the board
usb_protrude = 0.9;    // USB-C receptacle sticking out past the board edge
usb_z = 1.6;           // USB-C centre above the board's top surface
plug_w = 12.0;         // USB-C plug overmold of your cable
plug_h = 6.4;
// Positions on the board, seen from the top with the USB-C on the left:
// x = distance from the USB edge, y = distance from the bottom (front) edge.
led_x = 8.5;           // board LED centre
led_y = 9.0;
led_h = 1.0;           // LED height above the board
boot_x = 8.5;          // BOOT button centre, same reference as the LED
boot_y = 5.0;
boot_h = 1.6;          // BOOT button top above the board

/* [3.5 mm panel jack] */
thread_d = 7.7;        // thread outer diameter
thread_l = 5.0;        // thread length (panel + nut must fit on it)
body_d = 10.0;          // body behind the panel
body_l = 17.5;         // body length including the solder lugs
wire_space = 6.0;      // room for the wires between board and jack lugs

/* [Case] */
wall = 2.0;
floor_t = 1.6;
lid_t = 1.6;
clr = 0.4;             // clearance around parts
board_clr = 0.15;      // clearance around the board (it must not rattle)
lip_h = 3.0;           // lid lip that drops inside the long walls
lip_t = 1.2;
snap = 0.5;            // snap bump height
pipe_d = 3.0;          // light pipe
corner_r = 2.0;
$fn = 48;

// ---- derived ----
inner_l = usb_protrude + board_clr + board_l + wire_space + body_l + clr;
inner_w = max(board_w + 2 * board_clr, body_d + 2 * clr);
board_z = under_h;                      // board bottom above the inner floor
board_top = board_z + board_t;
inner_h = max(board_top + comp_h + 1.0, body_d + 2 * clr + 1.0);
outer_l = inner_l + 2 * wall;
outer_w = inner_w + 2 * wall;
outer_h = floor_t + inner_h;
board_x0 = usb_protrude + board_clr;    // board USB edge, from the USB wall's inner face
board_y0 = (inner_w - board_w) / 2;
jack_z = inner_h / 2;
jack_y = inner_w / 2;

module rbox(l, w, h, r) {
  hull() for (x = [r, l - r], y = [r, w - r]) translate([x, y, 0]) cylinder(r = r, h = h);
}

// Inner coordinates: origin at the inner floor corner on the USB side.
module at_inner() { translate([wall, wall, floor_t]) children(); }

module base() {
  difference() {
    rbox(outer_l, outer_w, outer_h, corner_r);
    at_inner() {
      cube([inner_l, inner_w, inner_h + 1]);
      // USB-C plug opening
      translate([-wall - 1, board_y0 + board_w / 2 - (plug_w + 1) / 2, board_top + usb_z - (plug_h + 1) / 2])
        cube([wall + 2, plug_w + 1, plug_h + 1]);
      // jack thread hole
      translate([inner_l - 1, jack_y, jack_z]) rotate([0, 90, 0]) cylinder(d = thread_d + clr, h = wall + 2);
      // snap grooves in the long walls
      for (y = [-snap, inner_w])
        translate([inner_l * 0.2, y, inner_h - lip_h / 2 - 0.6]) cube([inner_l * 0.6, snap, 1.2]);
    }
  }
  at_inner() {
    // board supports in the middle and under the corners (no tilting); the edge pads have
    // solder joints underneath, so the corner pads stay small
    for (x = [board_x0 + board_l * 0.3, board_x0 + board_l * 0.75])
      translate([x - 2, inner_w / 2 - 2, 0]) cube([4, 4, board_z]);
    for (x = [board_x0, board_x0 + board_l - 2], y = [board_y0, board_y0 + board_w - 2])
      translate([x, y, 0]) cube([2, 2, board_z]);
    // end stops behind the board, so pushing the USB plug in cannot shove it back
    for (y = [board_y0 + 2, board_y0 + board_w - 4])
      translate([board_x0 + board_l + board_clr, y, 0]) cube([1.6, 2, board_top + 1]);
    // stops at the USB end, beside the receptacle: the receptacle is smaller than the plug
    // opening and would otherwise let the board slide into it
    for (y = [board_y0, board_y0 + board_w - 3.5])
      translate([0, y, 0]) cube([board_x0 - board_clr, 3.5, board_top + 1]);
  }
}

// Lid pins pressing the board down at three corners: not the USB corner with the wires.
hold_pts = [[board_l - 1.5, 1.5], [board_l - 1.5, board_w - 1.5], [1.5, board_w - 1.5]];

module lid() {
  difference() {
    union() {
      rbox(outer_l, outer_w, lid_t, corner_r);
      // lip with snap bumps along both long walls
      for (y = [wall + clr / 2, wall + inner_w - lip_t - clr / 2]) {
        translate([wall + inner_l * 0.15, y, lid_t]) cube([inner_l * 0.7, lip_t, lip_h]);
        translate([wall + inner_l * 0.2, y < outer_w / 2 ? y - snap + 0.01 : y + lip_t - 0.01, lid_t + lip_h / 2 - 0.5])
          cube([inner_l * 0.6, snap, 1.0]);
      }
      // pins holding the board down (0.1 mm press)
      for (p = hold_pts)
        translate([wall + board_x0 + p[0], outer_w - wall - board_y0 - p[1], lid_t])
          cylinder(d = 2.5, h = inner_h - board_top + 0.1);
      // tube around the light pipe, ending just above the LED
      translate([wall + board_x0 + led_x, outer_w - wall - board_y0 - led_y, lid_t])
        cylinder(d = pipe_d + 2.4, h = inner_h - board_top - led_h - 0.5);
    }
    // light pipe hole
    translate([wall + board_x0 + led_x, outer_w - wall - board_y0 - led_y, -1])
      cylinder(d = pipe_d + 0.2, h = inner_h + 2);
    // BOOT pin hole
    translate([wall + board_x0 + boot_x, outer_w - wall - board_y0 - boot_y, -1]) cylinder(d = 2.0, h = lid_t + 2);
  }
}

// The lid is printed upside down; mirrored in y so lid holes line up over the board.
pipe_l = lid_t + inner_h - board_top - led_h - 0.3;
module pipe() { cylinder(d = pipe_d, h = pipe_l); }

// Fit check: the base with stand-ins for the board, USB-C, LED, BOOT and jack, and the lid
// lifted above it in its mounted orientation.
module fit() {
  base();
  at_inner() {
    color("green") translate([board_x0, board_y0, board_z]) cube([board_l, board_w, board_t]);
    color("silver") translate([board_x0 - usb_protrude, board_y0 + board_w / 2 - 4.5, board_top])
      cube([7.5, 9, 3.2]);
    color("white") translate([board_x0 + led_x - 1, board_y0 + led_y - 1, board_top]) cube([2, 2, led_h]);
    color("orange") translate([board_x0 + boot_x - 1.5, board_y0 + boot_y - 1.5, board_top]) cube([3, 3, boot_h]);
    color("dimgray") translate([inner_l - body_l, jack_y, jack_z]) rotate([0, 90, 0]) cylinder(d = body_d - 1, h = body_l);
    color("gold") translate([inner_l, jack_y, jack_z]) rotate([0, 90, 0]) cylinder(d = thread_d, h = thread_l);
  }
  translate([0, outer_w, outer_h + lid_t + 12]) rotate([180, 0, 0]) lid();
  color("lightblue") translate([wall + board_x0 + led_x, wall + board_y0 + led_y, outer_h + 12 - pipe_l + lid_t])
    pipe();
}

if (part == "fit") fit();
else if (part == "base") base();
else if (part == "lid") lid();
else if (part == "pipe") pipe();
else {
  base();
  translate([0, outer_w + 8, 0]) lid();
  translate([outer_l + 8, outer_w / 2, 0]) pipe();
}

echo(str("outer size: ", outer_l, " x ", outer_w, " x ", outer_h + lid_t, " mm"));
