[gcode_macro _Y_EXTRUDER_LOAD]
description: Loads the filament from Y-splitter to the Extruder
gcode:
    {% set svv = printer.save_variables.variables %}
    {% set chameleon = svv.chameleon %}
    {% set TOOL = chameleon[0] %}
    {% set YSPLIT_DIST = printer["gcode_macro CHAMELEON_HOME"].ysplit_extruder %}
    {% set SPEED = printer["gcode_macro CHAMELEON_HOME"].chameleon_speed %}
    {% set ACC = printer["gcode_macro CHAMELEON_HOME"].chameleon_acc %}
    {% set EXT_AMNT = printer["gcode_macro CHAMELEON_HOME"].extrude_amount %}
    
    _TOOL_SELECT TOOL={TOOL}
    
    # The extruder needs to move the full distance from Y-splitter to nozzle
    {% set TOTAL_DISTANCE = YSPLIT_DIST + EXT_AMNT %}
    
    SYNC_MOVE TOOL={TOOL} DISTANCE={TOTAL_DISTANCE} SPEED={SPEED} ACCEL={ACC}
    
    {% set chameleon = chameleon[:6] + [chameleon[0]] %}
    SAVE_VARIABLE VARIABLE=chameleon VALUE="{chameleon}"
    M118 Load tool {TOOL}: Moved {TOTAL_DISTANCE}mm
    CHAMELEON_CHK_ACTIVE_TOOL



























[gcode_macro CHAMELEON_HOME]
description: homes the tool selector. Needed before any tool change.
variable_ysplit_extruder: 1120
variable_chameleon_ysplit: 340
variable_chameleon_speed: 300
variable_chameleon_acc: 100
variable_extrude_amount: 120
variable_chameleon_is_homed: 0
gcode:
  {% set svv = printer.save_variables.variables %}
  # Initialize or update the chameleon parameter
  {% if 'chameleon' not in svv %}
    {% set chameleon = [0, -1, 'empty', 'empty', 'empty', 'empty', -1] %}
  {% else %}
    {% set chameleon = svv.chameleon %}
    {% if chameleon[0] != -1 %}
      {% set chameleon = [chameleon[0]] + [chameleon[0]] + chameleon[2:] %}   #set previous=current
    {% else %}
      {% set chameleon = [chameleon[0]] + chameleon[1:] %}             #preserve previous state if current=-1 
    {% endif %}
  {% endif %}
  MANUAL_STEPPER STEPPER=selector SET_POSITION=0 
  MANUAL_STEPPER STEPPER=selector MOVE=-2.65 STOP_ON_ENDSTOP=1
  MANUAL_STEPPER STEPPER=selector SET_POSITION=0 
  MANUAL_STEPPER STEPPER=selector MOVE=0.12
  MANUAL_STEPPER STEPPER=selector SET_POSITION=0 
  MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0 
  # Set current tool to 0 after homing
  {% set chameleon = [0] + chameleon[1:] %}
  SAVE_VARIABLE VARIABLE=chameleon VALUE="{chameleon}"
  SET_GCODE_VARIABLE MACRO=CHAMELEON_HOME VARIABLE=chameleon_is_homed VALUE=1
  M118 Chameleon is Homed!
  CHAMELEON_CHK_ACTIVE_TOOL
  
[gcode_macro CHAMELEON_TS0]
gcode:
  _TOOL_SELECT TOOL=0
  CHAMELEON_CHK_ACTIVE_TOOL

[gcode_macro CHAMELEON_TS1]
gcode:
  _TOOL_SELECT TOOL=1
  CHAMELEON_CHK_ACTIVE_TOOL

[gcode_macro CHAMELEON_TS2]
gcode:
  _TOOL_SELECT TOOL=2
  CHAMELEON_CHK_ACTIVE_TOOL

[gcode_macro CHAMELEON_TS3]
gcode:
  _TOOL_SELECT TOOL=3
  CHAMELEON_CHK_ACTIVE_TOOL

[gcode_macro _TOOL_SELECT]
gcode:
  {% set TOOL = params.TOOL|int %}
  {% if printer["gcode_macro CHAMELEON_HOME"].chameleon_is_homed != 1 or printer.stepper_enable.steppers['manual_stepper selector'] != True %}
    M118 Chameleon is not homed, must run CHAMELEON_HOME
  {% else %}
    {% set svv = printer.save_variables.variables %}
    {% set chameleon = svv.chameleon %}
    {% set chameleon = [TOOL, chameleon[0]] + chameleon[2:] %}
    {% if TOOL == 0 %}
      MANUAL_STEPPER stepper=selector MOVE=0
    {% elif TOOL == 1 %}
      MANUAL_STEPPER stepper=selector MOVE=0.5
    {% elif TOOL == 2 %}
      MANUAL_STEPPER stepper=selector MOVE=1.0
    {% elif TOOL == 3 %}
      MANUAL_STEPPER stepper=selector MOVE=1.5
    {% endif %}
    SAVE_VARIABLE VARIABLE=chameleon VALUE="{chameleon}"
    M118 Chameleon is set to tool {TOOL}
  {% endif %}

[gcode_macro CHAMELEON_TOOL_FREE]
description: Free filament path from selector and disable selector
gcode:
  {% if printer["gcode_macro CHAMELEON_HOME"].chameleon_is_homed != 1 or printer.stepper_enable.steppers['manual_stepper selector'] != True %}
    M118 Chameleon is not homed, must run CHAMELEON_HOME
  {% else %}
    {% set svv = printer.save_variables.variables %}
    {% set chameleon = svv.chameleon %}
    {% set TOOL = chameleon[0] %}    
    # Update chameleon parameter
    {% set chameleon = [-1, TOOL] + chameleon[2:] %}
    SAVE_VARIABLE VARIABLE=chameleon VALUE="{chameleon}"    
    M118 Chameleon free filament path for tool {TOOL}
    {% if TOOL == 0 %}
      MANUAL_STEPPER stepper=selector MOVE=1 
    {% elif TOOL == 1 %}
      MANUAL_STEPPER stepper=selector MOVE=1.5
    {% elif TOOL == 2 %}
      MANUAL_STEPPER stepper=selector MOVE=0
    {% elif TOOL == 3 %}
      MANUAL_STEPPER stepper=selector MOVE=0.5
    {% endif %}
    MANUAL_STEPPER STEPPER=selector ENABLE=0               #disables steppers
    MANUAL_STEPPER STEPPER=chameleon ENABLE=0
    SET_GCODE_VARIABLE MACRO=CHAMELEON_HOME VARIABLE=chameleon_is_homed VALUE=0
  {% endif %}
  CHAMELEON_CHK_ACTIVE_TOOL

[gcode_macro CHAMELEON_Y_UNLOAD]
description: Unloads the filament from the Y-splitter and updates tip shaping requirements
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  {% set TOOL = chameleon[0] %}
  {% set YSPLIT_DIST = printer["gcode_macro CHAMELEON_HOME"].chameleon_ysplit %}
  {% set SPEED = printer["gcode_macro CHAMELEON_HOME"].chameleon_speed %}
  {% set ACC = printer["gcode_macro CHAMELEON_HOME"].chameleon_acc %}

  # Check if there's filament loaded in the current tool
  {% if chameleon[TOOL+2] == 'empty' %}
    M118 Tool {TOOL} is already empty. No unload necessary.
  {% else %}
    # Check if the system is homed
    {% if printer["gcode_macro CHAMELEON_HOME"].chameleon_is_homed != 1 or printer.stepper_enable.steppers['manual_stepper selector'] != True %}
      M118 Chameleon is not homed, must run CHAMELEON_HOME
    {% else %}
      # Perform unload operation
      MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
      {% if TOOL == 0 or TOOL == 1 %}
        MANUAL_STEPPER stepper=chameleon MOVE=-{YSPLIT_DIST+30} SPEED={SPEED} ACCEL={ACC}
      {% else %}
        MANUAL_STEPPER stepper=chameleon MOVE={YSPLIT_DIST+30} SPEED={SPEED} ACCEL={ACC}
      {% endif %}
      MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
      
      # Update the tool status to 'empty'
      {% set chameleon = chameleon[:TOOL+2] + ['empty'] + chameleon[TOOL+3:6] + [chameleon[6]] %}
      SAVE_VARIABLE VARIABLE=chameleon VALUE="{chameleon}"
      M118 Unload tool {TOOL} from Y-splitter by {YSPLIT_DIST+30}mm and set status to 'empty' for tool {TOOL}
    {% endif %}
  {% endif %}
  CHAMELEON_CHK_ACTIVE_TOOL

[gcode_macro CHAMELEON_LOAD_HARD]
description: Loads the normal hard filament into the Y-splitter and updates tip shaping requirements
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  {% set TOOL = chameleon[0] %}
  {% set YSPLIT_DIST = printer["gcode_macro CHAMELEON_HOME"].chameleon_ysplit %}
  {% set SPEED = printer["gcode_macro CHAMELEON_HOME"].chameleon_speed %}
  {% set ACC = printer["gcode_macro CHAMELEON_HOME"].chameleon_acc %}

  # Check if the tool is already loaded or if its status conflicts
  {% if chameleon[TOOL+2] == 'hard' %}
    M118 Tool {TOOL} is already loaded with hard filament. No action taken.
  {% elif chameleon[TOOL+2] != 'empty' %}
    M118 Error: Tool {TOOL} is not empty. Please unload it first.
  {% else %}
    # Update the tool status to 'hard'
    {% set chameleon = chameleon[:TOOL+2] + ['hard'] + chameleon[TOOL+3:6] + [chameleon[6]] %}
    SAVE_VARIABLE VARIABLE=chameleon VALUE="{chameleon}"
    MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
    {% if printer["gcode_macro CHAMELEON_HOME"].chameleon_is_homed != 1 or printer.stepper_enable.steppers['manual_stepper selector'] != True %}
      M118 Chameleon is not homed, must run CHAMELEON_HOME
    {% else %}
      {% if TOOL == 0 or TOOL == 1 %}
        MANUAL_STEPPER stepper=chameleon MOVE={YSPLIT_DIST} SPEED={SPEED} ACCEL={ACC}
      {% else %}
        MANUAL_STEPPER stepper=chameleon MOVE=-{YSPLIT_DIST} SPEED={SPEED} ACCEL={ACC}
      {% endif %}
      MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
      M118 Load tool {TOOL} to Y-splitter by {YSPLIT_DIST}mm and set status to 'hard' for tool {TOOL}
    {% endif %}
  {% endif %}
  CHAMELEON_CHK_ACTIVE_TOOL

[gcode_macro CHAMELEON_LOAD_SOFT]
description: Loads soft TPU filament into the Y-splitter and updates tip shaping requirements
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  {% set TOOL = chameleon[0] %}
  {% set YSPLIT_DIST = printer["gcode_macro CHAMELEON_HOME"].chameleon_ysplit %}
  {% set SPEED = printer["gcode_macro CHAMELEON_HOME"].chameleon_speed %}
  {% set ACC = printer["gcode_macro CHAMELEON_HOME"].chameleon_acc %}

  # Check if the tool is already loaded or if its status conflicts
  {% if chameleon[TOOL+2] == 'soft' %}
    M118 Tool {TOOL} is already loaded with soft filament. No action taken.
  {% elif chameleon[TOOL+2] != 'empty' %}
    M118 Error: Tool {TOOL} is not empty. Please unload it first.
  {% else %}
    # Update the tool status to 'soft'
    {% set chameleon = chameleon[:TOOL+2] + ['soft'] + chameleon[TOOL+3:6] + [chameleon[6]] %}
    SAVE_VARIABLE VARIABLE=chameleon VALUE="{chameleon}"
    MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
    {% if printer["gcode_macro CHAMELEON_HOME"].chameleon_is_homed != 1 or printer.stepper_enable.steppers['manual_stepper selector'] != True %}
      M118 Chameleon is not homed, must run CHAMELEON_HOME
    {% else %}
      {% if TOOL == 0 or TOOL == 1 %}
        MANUAL_STEPPER stepper=chameleon MOVE={YSPLIT_DIST} SPEED={SPEED} ACCEL={ACC}
      {% else %}
        MANUAL_STEPPER stepper=chameleon MOVE=-{YSPLIT_DIST} SPEED={SPEED} ACCEL={ACC}
      {% endif %}
      MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
      M118 Load tool {TOOL} to Y-splitter by {YSPLIT_DIST}mm and set status to 'soft' for tool {TOOL}
    {% endif %}
  {% endif %}
  CHAMELEON_CHK_ACTIVE_TOOL

[gcode_macro CHAMELEON_CHK_ACTIVE_TOOL]
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  RESPOND TYPE=command MSG='Current active tool is tool {chameleon[0]}'
  RESPOND TYPE=command MSG='Previous active tool was tool {chameleon[1]}'
  RESPOND TYPE=command MSG='Tool status:'
  {% for tool in range(4) %}
    RESPOND TYPE=command MSG='Tool {tool}: {chameleon[tool+2]}'
  {% endfor %}
  RESPOND TYPE=command MSG='Tool loaded to extruder: {chameleon[6]}'

[gcode_macro _EXTRUDER_Y_UNLOAD]
description: Unloads the filament from the Extruder to Y-splitter 
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  {% set TOOL = chameleon[1] %}
  {% set YSPLIT_DIST = printer["gcode_macro CHAMELEON_HOME"].ysplit_extruder %}
  {% set SPEED = printer["gcode_macro CHAMELEON_HOME"].chameleon_speed %}
  {% set ACC = printer["gcode_macro CHAMELEON_HOME"].chameleon_acc %}
  CHAMELEON_HOME
  _TOOL_SELECT TOOL=TOOL
  MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
  {% if TOOL == 0 or TOOL == 1 %}
    MANUAL_STEPPER stepper=chameleon MOVE=-{YSPLIT_DIST} SPEED={SPEED} ACCEL={ACC}
  {% else %}
    MANUAL_STEPPER stepper=chameleon MOVE={YSPLIT_DIST} SPEED={SPEED} ACCEL={ACC}
  {% endif %}
  MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
  M118 Unload tool {TOOL} from Extruder to Y-splitter by {YSPLIT_DIST}mm 
  CHAMELEON_CHK_ACTIVE_TOOL





# [gcode_macro _Y_EXTRUDER_LOAD]
# description: Loads the filament from Y-splitter to the Extruder must be run with the tool argument
# gcode:
#   {% set svv = printer.save_variables.variables %}
#   {% set chameleon = svv.chameleon %}
#   {% set TOOL = chameleon[0] %} #Set the tool to the selected tool current 
#   {% set YSPLIT_DIST = printer["gcode_macro CHAMELEON_HOME"].ysplit_extruder %} # Distance to reach the Extruder Gear
#   {% set SPEED = printer["gcode_macro CHAMELEON_HOME"].chameleon_speed %}
#   {% set ACC = printer["gcode_macro CHAMELEON_HOME"].chameleon_acc %}
#   {% set EXT_AMNT=printer["gcode_macro CHAMELEON_HOME"].extrude_amount %}
#   _TOOL_SELECT TOOL=TOOL
#   MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
#   {% set FILAMENT_TYPE = chameleon[TOOL + 2] %}
#   {% if FILAMENT_TYPE == "hard" %}
#     {% if TOOL == 0 or TOOL == 1 %}
#       MANUAL_STEPPER stepper=chameleon MOVE={YSPLIT_DIST+30} SPEED={SPEED} ACCEL={ACC} STOP_ON_ENDSTOP=1
#       SYNC_MOVE_CHAMELEON_EXTRUDER DISTANCE=EXT_AMNT SPEED=SPEED TOOL=TOOL
#     {% else %}
#       MANUAL_STEPPER stepper=chameleon MOVE=-{YSPLIT_DIST+30} SPEED={SPEED} ACCEL={ACC} STOP_ON_ENDSTOP=1
#       SYNC_MOVE_CHAMELEON_EXTRUDER DISTANCE=EXT_AMNT SPEED=SPEED TOOL=TOOL
#     {% endif %}
#   {% elif FILAMENT_TYPE == "soft" %}
#     {% if TOOL == 0 or TOOL == 1 %}
#       MANUAL_STEPPER stepper=chameleon MOVE={YSPLIT_DIST} SPEED={SPEED} ACCEL={ACC}
#       SYNC_MOVE_CHAMELEON_EXTRUDER DISTANCE=EXT_AMNT SPEED=SPEED TOOL=TOOL
#     {% else %}
#       MANUAL_STEPPER stepper=chameleon MOVE=-{YSPLIT_DIST} SPEED={SPEED} ACCEL={ACC}
#       SYNC_MOVE_CHAMELEON_EXTRUDER DISTANCE=EXT_AMNT SPEED=SPEED TOOL=TOOL
#     {% endif %}
#   {% endif %}
#   MANUAL_STEPPER STEPPER=chameleon SET_POSITION=0
#   {% set chameleon = chameleon[:6] + [chameleon[0]] %}
#   SAVE_VARIABLE VARIABLE=chameleon VALUE="{chameleon}"
#   M118 Load tool {TOOL} from Y-splitter to Extruder by {YSPLIT_DIST}mm 
#   CHAMELEON_CHK_ACTIVE_TOOL
  
[gcode_macro EXTRUDER_UNLOAD]
description: Unloads the filament from the extruder before a tool change
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  {% set CURRENT_TOOL = chameleon[0] %}
  
  {% if CURRENT_TOOL == -1 %}
    {% set PREVIOUS_TOOL = chameleon[1] %}
    {% set FILAMENT_TYPE = chameleon[PREVIOUS_TOOL + 2] %}
  {% else %}
    {% set FILAMENT_TYPE = chameleon[CURRENT_TOOL + 2] %}
  {% endif %}
  
  {% if FILAMENT_TYPE == "hard" %}
    {% set moves = params.MOVES | default(5) | int %}
    M83
    {% for _ in range(moves) %}
      G92 E0
      G1 E10 F1500
      G92 E0
      G1 E-15 F1500
    {% endfor %}
    G1 E-45 F2000
    # from gears to SB TOP 45mm, from gears to beginning of melting zone on bottom 61mm.
  {% else %}
    {% set temp = params.TEMP | int %}
    {% set stages = params.STAGES | default(4) | int %}
    M106 S255
    {% if stages >= 1 %}
      M104 S180 ; cool down to prevent swelling
      TEMPERATURE_WAIT SENSOR=extruder MAXIMUM=180
    {% endif %}
    G0 E20 F1500
    G0 E-5 F500
    {% if stages >= 2 %}
      M104 S165
      TEMPERATURE_WAIT SENSOR=extruder MAXIMUM=165
    {% endif %}
    G0 E5 F1500
    G0 E-1 F500
    {% if stages >= 3 %}
      M104 S155
      TEMPERATURE_WAIT SENSOR=extruder MAXIMUM=155
    {% endif %}
    G0 E1 F1500
    G0 E-25 F500
    {% if stages >= 4 %}
      M104 S150
      TEMPERATURE_WAIT SENSOR=extruder MAXIMUM=150
    {% endif %}
    G0 E24 F1500 ; last tip dip with cold tip
    G0 E-24 ; last tip dip with cold tip
    M109 R180 ; ok... go back up in temp so we can move the extruder
    G0 E-80 F500 ; back out of the extruder
    G92 E0
    M107
    M104 S{temp}
  {% endif %}
  
  {% set chameleon = chameleon[:6] + [-1] %}                 #needs checking
  SAVE_VARIABLE VARIABLE=chameleon VALUE="{chameleon}"
  
  CHAMELEON_CHK_ACTIVE_TOOL

[gcode_macro T0]
description: Changes the filament to that loaded in tool 0
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  {% set CURRENT_TOOL = chameleon[0] %}
  {% set LOADED_TOOL = chameleon[6] %}

  CHAMELEON_CHK_ACTIVE_TOOL
  {% if LOADED_TOOL != -1 and LOADED_TOOL != 0 %}
    PAUSE
    EXTRUDER_UNLOAD
    _EXTRUDER_Y_UNLOAD
    _Y_EXTRUDER_LOAD TOOL=0
    EXTRUDER_LOAD
    WIPE
    RESUME
    CHAMELEON_CHK_ACTIVE_TOOL
  {% else %}
    M118 T0 is already loaded!
  {% endif %}

[gcode_macro T1]
description: Changes the filament to that loaded in tool 0
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  {% set CURRENT_TOOL = chameleon[0] %}
  {% set LOADED_TOOL = chameleon[6] %}

  CHAMELEON_CHK_ACTIVE_TOOL
  {% if LOADED_TOOL != -1 and LOADED_TOOL != 1 %}
    PAUSE
    EXTRUDER_UNLOAD
    _EXTRUDER_Y_UNLOAD
    _Y_EXTRUDER_LOAD TOOL=1
    EXTRUDER_LOAD
    WIPE
    RESUME
    CHAMELEON_CHK_ACTIVE_TOOL
  {% else %}
    M118 T1 is already loaded!
  {% endif %}

[gcode_macro T2]
description: Changes the filament to that loaded in tool 0
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  {% set CURRENT_TOOL = chameleon[0] %}
  {% set LOADED_TOOL = chameleon[6] %}

  CHAMELEON_CHK_ACTIVE_TOOL
  {% if LOADED_TOOL != -1 and LOADED_TOOL != 2 %}
    PAUSE
    EXTRUDER_UNLOAD
    _EXTRUDER_Y_UNLOAD
    _Y_EXTRUDER_LOAD TOOL=2
    EXTRUDER_LOAD
    WIPE
    RESUME
    CHAMELEON_CHK_ACTIVE_TOOL
  {% else %}
    M118 T2 is already loaded!
  {% endif %}

[gcode_macro T3]
description: Changes the filament to that loaded in tool 0
gcode:
  {% set svv = printer.save_variables.variables %}
  {% set chameleon = svv.chameleon %}
  {% set CURRENT_TOOL = chameleon[0] %}
  {% set LOADED_TOOL = chameleon[6] %}

  CHAMELEON_CHK_ACTIVE_TOOL
  {% if LOADED_TOOL != -1 and LOADED_TOOL != 3 %}
    PAUSE
    EXTRUDER_UNLOAD
    _EXTRUDER_Y_UNLOAD
    _Y_EXTRUDER_LOAD TOOL=3
    EXTRUDER_LOAD
    WIPE
    RESUME
    CHAMELEON_CHK_ACTIVE_TOOL
  {% else %}
    M118 T3 is already loaded!
  {% endif %}