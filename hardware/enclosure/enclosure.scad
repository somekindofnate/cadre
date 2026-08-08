/*
  Handheld Drone Detector Enclosure - Extended (+2")
  Hardware: ESP32S2, Heltec (w/ Screen), BW16, USB Power Bank
  Mechanism: 4x Cantilever Snap Joints
*/

$fn = 60; // Smoothness for circular cutouts

// --- Printer Tolerances & Settings ---
wall = 2.0;       // Standard 2mm shell thickness
tol = 0.15;       // Clearance tolerance for the alignment lip

// --- Snap Joint Parameters ---
snap_w = 10.0;      // Width of the snap arm
snap_t = 1.8;       // Thickness of the cantilever arm
snap_l = 7.0;       // Length of the snap extending past the rim
snap_h = 1.2;       // Hook protrusion depth

// --- Internal Dimensions (mm) ---
// Battery: 136.9 x 67.1 x 15 (Updated to exact 15mm thickness)
bat_w = 67.1;
bat_h = 136.9;
bat_d = 15.0;

usb_clearance = 58.8;       // 8mm original + 50.8mm (2 inches) for cable routing
int_w = bat_w + 2;          // 69.1mm wide internal cavity
int_h = bat_h + usb_clearance; // 195.7mm high total cavity
front_depth = 18.0;         // Depth for boards (Accounts for BW16 header pins)

// back_depth is bat_d + 8 to raise bottom walls & clasp receivers 
back_depth = bat_d + 8;     

// --- External Dimensions ---
ext_w = int_w + (wall * 2);
ext_h = int_h + (wall * 2);
ext_d = front_depth + back_depth + (wall * 2);

// --- Component Coordinates ---
// Screen rotated vertically, widened by ~6.35mm (0.25"), plus an extra 2mm in height
screen_w = 20;     // Original short edge
screen_h = 38.35;  // Original long edge (30) + 6.35mm (0.25") + 2.0mm

// Anchored from the top down, left justified with a buffer for the board
screen_x = wall + 10; // Left-justified 10mm from internal wall
screen_y = ext_h - 75; 

sma_dia = 6.5;        // ESP32S2 Antenna Port
foxeer_dia = 8.0;     // BW16 Antenna Port

// --- View Selector ---
// 0 = Exploded View, 1 = Front Face Only, 2 = Back Cover Only
part = 0; 

if (part == 0) {
    front_half();
    translate([ext_w + 12, 0, 0]) back_half();
} else if (part == 1) {
    front_half();
} else if (part == 2) {
    back_half();
}

// ==========================================
//                 MODULES
// ==========================================

module hook_left() {
    // Main cantilever arm
    cube([snap_t, snap_w, snap_l]);
    // The wedge ramp and catch (facing outward to the left)
    translate([-snap_h, 0, snap_l - 2])
        hull() {
            cube([0.1, snap_w, 0.1]);                       // Bottom catching ledge
            translate([snap_h, 0, 2]) cube([0.1, snap_w, 0.1]); // Top tip (flush with arm)
            translate([snap_h, 0, 0]) cube([0.1, snap_w, 0.1]); // Base of the wedge
        }
}

module hook_right() {
    // Main cantilever arm
    translate([-snap_t, 0, 0]) cube([snap_t, snap_w, snap_l]);
    // The wedge ramp and catch (facing outward to the right)
    translate([0, 0, snap_l - 2])
        hull() {
            translate([snap_h, 0, 0]) cube([0.1, snap_w, 0.1]); // Bottom catching ledge
            translate([0, 0, 2]) cube([0.1, snap_w, 0.1]);      // Top tip (flush with arm)
            cube([0.1, snap_w, 0.1]);                           // Base of the wedge
        }
}

module front_half() {
    union() {
        difference() {
            // Main Outer Block
            cube([ext_w, ext_h, front_depth + wall]);
            
            // Hollow Internal Cavity
            translate([wall, wall, wall])
                cube([int_w, int_h, front_depth + 1]);
                
            // Heltec Screen Cutout (Vertical & Left-Justified)
            translate([screen_x, screen_y - (screen_h/2), -1])
                cube([screen_w, screen_h, wall + 2]);
                
            // Top Face Antenna Ports
            translate([ext_w/3, ext_h + 1, wall + (front_depth/2)])
                rotate([90, 0, 0])
                cylinder(d=sma_dia, h=wall+2);
                
            translate([ext_w*2/3, ext_h + 1, wall + (front_depth/2)])
                rotate([90, 0, 0])
                cylinder(d=foxeer_dia, h=wall+2);
        }
        
        // 1mm Alignment Lip (Keeps the two halves from shearing sideways)
        translate([wall/2, wall/2, front_depth + wall])
            difference() {
                cube([ext_w - wall, ext_h - wall, 1]); 
                translate([wall/2, wall/2, -1])
                    cube([ext_w - wall*2, ext_h - wall*2, 3]);
            }
            
        // 4x Cantilever Snaps
        for (y = [ext_h*0.25, ext_h*0.75]) {
            translate([wall, y - snap_w/2, front_depth + wall])
                hook_left();
            translate([ext_w - wall, y - snap_w/2, front_depth + wall])
                hook_right();
        }
    }
}

module back_half() {
    difference() {
        // Main Outer Block
        cube([ext_w, ext_h, back_depth + wall]);
        
        // Hollow Battery Cavity
        translate([wall, wall, wall])
            cube([int_w, int_h, back_depth + 1]);
            
        // Female Alignment Groove
        translate([wall/2 - tol, wall/2 - tol, back_depth + wall - 1])
            difference() {
                cube([ext_w - wall + (tol*2), ext_h - wall + (tol*2), 2]);
                translate([wall/2 + (tol*2), wall/2 + (tol*2), -1])
                    cube([ext_w - wall*2 - (tol*4), ext_h - wall*2 - (tol*4), 4]);
            }
            
        // Snap Receiver Windows (Cut fully through the outer shell)
        for (y = [ext_h*0.25, ext_h*0.75]) {
            // Left window
            translate([-1, y - (snap_w+1)/2, back_depth + wall - snap_l - 1])
                cube([wall + 2, snap_w + 1, 3.5]);
            // Right window
            translate([ext_w - wall - 1, y - (snap_w+1)/2, back_depth + wall - snap_l - 1])
                cube([wall + 2, snap_w + 1, 3.5]);
        }
            
        // Horizontal Vents (Full length of the back)
        // Spaced every 6mm, starting from 15mm up to near the top
        for (i = [0 : floor((ext_h - 30) / 6)]) {
            translate([wall + 10, 15 + (i * 6), -1])
                cube([ext_w - 20 - (wall*2), 2.5, wall + 2]);
        }
    }
}