let util = import "util_functions.ncl" in
let { Set, Uint8, Uint16, Uint32, .. } = import "util_types.ncl" in
let { tap, hold, .. } = import "keycode.ncl" in

fun kb km side =>

let virtual_keys_of_type = fun t =>
  km.virtual_keys
  |> std.array.filter (fun { type, .. } => type == t)
in

let combos = virtual_keys_of_type 'combo in
let combo_count = std.array.length combos in

let soft_serial_pin = if side == 'self then -1 else
  if (std.is_number kb.split.channel) then kb.split.channel else -1
in

# We can assume all encoders in `kb` are used, as preprocessed by `main.ncl`
let encoder_count = std.array.length kb.encoders in
let led_count = std.array.length kb.leds in

let physical_key_count = std.array.length kb.keys in
let virtual_key_count = std.array.length km.virtual_keys in
let key_count = physical_key_count + virtual_key_count in

let virtual_key_idx_start = physical_key_count in

let keycodes = std.array.flatten km.layers in
let layer_count = std.array.length km.layers in

let keycode_is_hold_tap = fun kc => 
  kc.type == 'hold_tap
  && kc.data.tap.type != 'none
  && kc.data.hold.type != 'none
  && kc.data.hold.type != 'transparent in

let keycode_is_macro = fun kc =>
  kc.type == 'hold_tap
  && kc.data.tap.type == 'custom
  && kc.data.tap.data.type == 'macro in

let keycodes_in_macro = fun kc =>
  kc.data.tap.data.data.steps
  |> std.array.filter (fun step => std.record.has_field "keycode" step.arg)
  |> std.array.map (fun step => step.arg.keycode) in

let deep_keycodes = std.array.flat_map (fun kc => match {
  'hold_tap => [kc] @ (if keycode_is_macro kc then keycodes_in_macro kc else []),
  'tap_dance => kc.data.bindings
} kc.type) keycodes in

let features_used =
  if side == 'self then [] else 
    [kb.split.channel]
    |> std.array.filter (fun e => std.is_record e)
in

let feature_pins_used = features_used
  |> std.array.flat_map (fun feature => feature.pins)
in

let feature_keys_used | Set String = features_used
  |> std.array.flat_map (fun feature => feature.keys)
in

let encoder_pins_used = kb.encoders
  |> std.array.filter ( fun { type, .. } => type == 'physical)
  |> std.array.flat_map (fun { data, .. } => [data.pin_a, data.pin_b])
in

let _hold_tap_behaviors =
  let hold_tap_keycodes = std.array.filter keycode_is_hold_tap deep_keycodes in
  let all_behaviors = std.array.map (fun kc => kc.data.hold.data.behavior) hold_tap_keycodes in
  util.array.unique all_behaviors in

let _tap_dance_bindings =
  let tap_dance_keycodes = std.array.filter (fun kc => kc.type == 'tap_dance) keycodes in
  let all_bindings = std.array.map (fun kc => kc.data.bindings) tap_dance_keycodes in
  util.array.unique all_bindings in

let raw_macro_steps =
  deep_keycodes
  |> std.array.filter keycode_is_macro
  |> std.array.map (fun kc => kc.data.tap.data.data.steps)
  |> util.array.unique in

let index_of_macro_steps = fun steps =>
  raw_macro_steps
  |> util.array.index_of steps
  # We add 1 for the halt step
  |> std.array.generate (fun i => 1 + std.array.length (std.array.at i raw_macro_steps))
  |> std.array.fold_left (+) 0
in

let encode_modifiers = fun mods => std.array.reduce_left (+) (std.array.map (fun { field, value } => 
  let M = [
    "left_ctrl", "left_shift", "left_alt", "left_gui",
    "right_ctrl", "right_shift", "right_alt", "right_gui",
  ] in
  (if value == true then 1 else 0) * (std.number.pow 2 (util.array.index_of field M))
) (std.record.to_array mods)) in

let encode_tappable = fun { type, data } => match {
  'none => 0,
  'regular => data.hid_code + util.bit.shift (encode_modifiers data.mods) 8,
  'sticky => 160 + (util.bit.shift (encode_modifiers data.mods) 8) + data.layer,
  'layer => 
    let op = match { 'df => 0, 'tog => 1, 'to => 2 } data.operation in
    192 + data.layer + util.bit.shift op 8,
  'custom =>
    let code = match {
      'macro => index_of_macro_steps data.data.steps,
      _ => data.data.code,
    } data.type in
    let m = util.bit.shift (util.bit.shift code (-2)) 2 in
    224 + util.bit.shift (code - m) 3 + util.bit.shift m 6 + match {
      'fak => 0,
      'consumer => 1,
      'user => 2,
      'mouse => 3,
      'macro => 4,
    } data.type,
  'transparent_layer_exit => 65534,
  'transparent => 65535
} type in

let encode_holdable = fun { type, data } is_hold_tap =>
  let behavior = fun data => util.bit.shift (util.array.index_of data.behavior _hold_tap_behaviors) 13 in
  match {
    'none => 0,
    'regular =>
      (if is_hold_tap then (behavior data) else 0)
      + util.bit.shift data.layer 8
      + encode_modifiers data.mods,
    'nop => 8191 + behavior data,
    'transparent_layer_exit => 8190 + behavior data,
    'transparent => 65535
  } type
in

let encode_hold_tap = fun { tap, hold } is_hold_tap =>
  encode_tappable tap
  + util.bit.shift (encode_holdable hold is_hold_tap) 16
in

let encode_tap_dance = fun { tapping_term_ms, bindings, max_taps } =>
  let binding_start = _tap_dance_bindings
    |> util.array.index_of bindings
    |> std.array.generate (fun i => std.array.length (std.array.at i _tap_dance_bindings))
    |> std.array.fold_left (+) 0
  in
  util.bit.shift 14 28
  + util.bit.shift max_taps 20
  + util.bit.shift tapping_term_ms 8
  + (binding_start | Uint8) in

let encode_kc = fun kc => match {
  'hold_tap => encode_hold_tap kc.data (keycode_is_hold_tap kc),
  'tap_dance => encode_tap_dance kc.data,
} kc.type in

let encode_hold_tap_key_interrupt = fun { decision, trigger_on } => 
  if decision == 'none then 0 else (
    1 + (if decision == 'hold then 2 else 0)
    + (if trigger_on == 'press then 4 else 0)
  ) in

let encode_hold_tap_key_interrupts = fun key_interrupts =>
  let encode_pair = (fun pair =>
    (std.array.at 0 pair) + (util.bit.shift (util.array.last_or 0 pair) 4)
  ) in
  key_interrupts
  |> std.array.map encode_hold_tap_key_interrupt
  |> util.array.chunk 2
  |> std.array.map encode_pair
in

let encode_usb_str = fun str =>
  if std.string.length str > 0 then
    str
    |> std.string.characters
    |> std.array.flat_map (fun c => ["'%{c}'", "0"])
    |> util.array.join ","
  else
    false
in

let is_custom_keys_used =
  deep_keycodes
  |> std.array.any (fun kc => 
      kc.type == 'hold_tap
      && kc.data.tap.type == 'custom)
in

let is_custom_keys_of_type_used = fun type =>
  deep_keycodes
  |> std.array.any (fun kc => 
      kc.type == 'hold_tap
      && kc.data.tap.type == 'custom
      && kc.data.tap.data.type == type)
in

let is_custom_keys_of_type_of_keycodes_used = fun type keycodes =>
  deep_keycodes
  |> std.array.any (fun kc =>
      kc.type == 'hold_tap
      && kc.data.tap.type == 'custom
      && kc.data.tap.data.type == type
      && std.array.elem kc.data.tap.data.data.code keycodes)
in

let encode_macro_step_arg = fun step =>
  match {
    'wait => step.arg.duration_ms,
    'pause_for_release => 0,
    _ => encode_kc step.arg.keycode,
  } step.inst in

let _macro_step_args =
  raw_macro_steps
  |> std.array.flatten
  |> std.array.map encode_macro_step_arg
  |> util.array.unique in

let _macro_steps =
  let halt_step = { inst = 0 } in
  let encode_step = fun step => {
    inst | Uint8 = match {
      'press => 1,
      'release => 2,
      'tap => 3,
      'wait => 4,
      'pause_for_release => 5,
    } step.inst,
    arg_idx | Uint16 = util.array.index_of (encode_macro_step_arg step) _macro_step_args,
  } in

  raw_macro_steps
  |> std.array.map (std.array.map encode_step)
  |> std.array.map (fun steps => steps @ [halt_step])
  |> std.array.flatten
in

let _central_defines = {
  KEY_COUNT = key_count,
  LAYER_COUNT = layer_count,
  DEBOUNCE_MS = kb.debounce_ms,

  LAYER_TRANSPARENCY_ENABLE = layer_count > 1 && (std.array.any (fun kc => 
    kc.type == 'hold_tap
    && (kc.data.tap.type == 'transparent || kc.data.hold.type == 'transparent)
  ) deep_keycodes),

  TRANS_LAYER_EXIT_ENABLE = layer_count > 1 && (std.array.any (fun kc => 
    kc.type == 'hold_tap
    && (kc.data.tap.type == 'transparent_layer_exit || kc.data.hold.type == 'transparent_layer_exit)
  ) deep_keycodes),

  HOLD_TAP_ENABLE = std.array.length _hold_tap_behaviors > 0,
  HOLD_TAP_EAGER_ENABLE = std.array.any (fun b => b.eager_decision != 'none) _hold_tap_behaviors,
  HOLD_TAP_QUICK_TAP_ENABLE = std.array.any (fun b => b.quick_tap_ms > 0) _hold_tap_behaviors,
  HOLD_TAP_QUICK_TAP_INTERRUPT_ENABLE = std.array.any (fun b => b.quick_tap_interrupt_ms > 0) _hold_tap_behaviors,
  HOLD_TAP_GLOBAL_QUICK_TAP_ENABLE = std.array.any (fun b => b.global_quick_tap_ms > 0) _hold_tap_behaviors,
  HOLD_TAP_GLOBAL_QUICK_TAP_IGNORE_CONSECUTIVE_ENABLE = std.array.any (fun b => b.global_quick_tap_ignore_consecutive) _hold_tap_behaviors,

  TAP_DANCE_ENABLE = std.array.any (fun kc => kc.type == 'tap_dance) keycodes,

  STICKY_ENABLE = std.array.any (fun kc => kc.type == 'hold_tap && kc.data.tap.type == 'sticky) deep_keycodes,

  CUSTOM_KEYS_ENABLE = is_custom_keys_used,
  FAK_KEYS_ENABLE = is_custom_keys_of_type_used 'fak,
  CONSUMER_KEYS_ENABLE = is_custom_keys_of_type_used 'consumer,
  USER_KEYS_ENABLE = is_custom_keys_of_type_used 'user,

  MOUSE_KEYS_ENABLE = is_custom_keys_of_type_used 'mouse,
  MOUSE_MOVE_SPEED = km.mouse.move_speed,
  MOUSE_SCROLL_INTERVAL_MS = km.mouse.scroll_interval_ms,

  MACRO_KEYS_ENABLE = std.array.length _macro_steps > 0,
  MACRO_STEP_ARG_COUNT = std.array.length _macro_step_args,

  CONDITIONAL_LAYER_COUNT = std.record.length km.conditional_layers,

  CAPS_WORD_ENABLE = is_custom_keys_of_type_of_keycodes_used 'fak [2, 3, 4],
  REPEAT_KEY_ENABLE = is_custom_keys_of_type_of_keycodes_used 'fak [5],

  COMBO_COUNT = combo_count,
  COMBO_REQUIRE_PRIOR_IDLE_MS_ENABLE = combos
    |> std.array.any (fun c => c.data.require_prior_idle_ms > 0),
  
  ENCODER_COUNT = encoder_count,

  LED_COUNT = std.array.length kb.leds,
  NEOPIXEL_ENABLE = kb.leds 
    |> std.array.any (fun led => led.type == 'ws2812b),
  NEOPIXEL_COUNT  = kb.leds 
    |> std.array.filter (fun led => led.type == 'ws2812b)
    |> std.array.map (fun led => led.data.num )
    |> std.array.fold_right (std.number.max) 0,
  NEOPIXEL_GRB    = true,
  NEOPIXEL_PIN    = kb.leds
    |> util.array.enumerate
    |> std.array.filter (fun { index, value } => value.type == 'ws2812b && value.data.num == 1)
    |> std.array.first # TODO: allow more than one neopixel string
    |> (fun { index, .. } => "_LED%{std.to_string index}"),

  USB_NUM_INTERFACES = [true, CONSUMER_KEYS_ENABLE, MOUSE_KEYS_ENABLE]
    |> std.array.filter ((==) true)
    |> std.array.length,

  USB_VENDOR_ID = kb.usb_dev.vendor_id,
  USB_PRODUCT_ID = kb.usb_dev.product_id,
  USB_PRODUCT_VER = kb.usb_dev.product_ver,

  # FIXME: Using strings make USB negotiation slow and I have no idea why
  USB_MANUFACTURER_STR = encode_usb_str kb.usb_dev.manufacturer,
  USB_PRODUCT_STR = encode_usb_str kb.usb_dev.product,
  USB_SERIAL_NO_STR = encode_usb_str kb.usb_dev.serial_number,

  USB_EP0_SIZE = 8,
  USB_EP1_SIZE = 8,
  USB_EP2_SIZE = 8,
  USB_EP3_SIZE = 4,
  KEY_EVENT_QUEUE_LEN = 32,
} & util.record.only_if (combo_count > 0) {
  COMBO_KEY_QUEUE_LEN = combos
    |> std.array.map (fun c => c.data.key_indices)
    |> std.array.reduce_left (@)
    |> util.array.unique
    |> std.array.length,
  COMBO_MAX_KEY_COUNT = combos
    |> std.array.map (fun c => std.array.length c.data.key_indices)
    |> util.array.max,
} in

let _xaddr_sizes = let
  sizeof = {
    usb_ep = fun i =>
      let name = "USB_EP%{std.to_string i}_SIZE" in
      let padding = 2 in
      _central_defines."%{name}" + padding,
    int8_t = 1,
    uint8_t = 1,
    uint16_t = 2,
    uint32_t = 4,
    fak_key_state_t = uint8_t + uint32_t,
    fak_key_event_t = (uint8_t * 3) + uint16_t,
  }
in {
  USB_EP0 = sizeof.usb_ep 0,
  USB_EP1 = sizeof.usb_ep 1,
  USB_TX_LEN = sizeof.uint8_t,
  LAST_TAP_TIMESTAMP = sizeof.uint16_t,
  KEY_STATES = sizeof.fak_key_state_t * _central_defines.KEY_COUNT,
  KEY_EVENT_QUEUE = (sizeof.uint8_t * 3) + (sizeof.fak_key_event_t * _central_defines.KEY_EVENT_QUEUE_LEN),
  STRONG_MODS_REF_COUNT = sizeof.uint8_t * 8,
}
& util.record.only_if _central_defines.CONSUMER_KEYS_ENABLE {
  USB_EP2 = sizeof.usb_ep 2,
}
& util.record.only_if _central_defines.MOUSE_KEYS_ENABLE {
  USB_EP3 = sizeof.usb_ep 3,
  MOUSE_SCROLL_DIRECTION = sizeof.int8_t,
  MOUSE_SCROLL_AT_TIME = sizeof.uint16_t,
}
& util.record.only_if (layer_count > 1) (
  let sizeof = sizeof & {
    fak_layer_state_t =
      if layer_count <= 8 then
        sizeof.uint8_t
      else if layer_count <= 16 then
        sizeof.uint16_t
      else
        sizeof.uint32_t
  } in
  {
    LAYER_STATE = sizeof.fak_layer_state_t,
    PERSISTENT_LAYER_STATE = sizeof.fak_layer_state_t,
  }
)
& util.record.only_if _central_defines.TAP_DANCE_ENABLE {
  TAP_COUNT = sizeof.uint8_t,
}
& util.record.only_if (combo_count > 0) (
  let sizeof = sizeof & {
    fak_combo_state_t = sizeof.uint8_t + sizeof.uint16_t,
    fak_combo_key_queue_entry_t = 2 * sizeof.uint8_t,
  } in
  {
    COMBO_STATES = sizeof.fak_combo_state_t * _central_defines.COMBO_COUNT, 
    COMBO_KEY_QUEUE = sizeof.uint8_t + (sizeof.fak_combo_key_queue_entry_t * _central_defines.COMBO_KEY_QUEUE_LEN),
  }
)
& util.record.only_if (encoder_count > 0) (
  {
    ENCODER_STEPS = sizeof.int8_t * encoder_count,
    ENCODER_LAST_NUM = sizeof.uint8_t * std.number.floor ((encoder_count + 3) / 4),
  }
)
& util.record.only_if _central_defines.STICKY_ENABLE {
  PENDING_STICKY_MODS = sizeof.uint8_t,
  APPLIED_STICKY_MODS = sizeof.uint8_t,
  APPLIED_STICKY_LAYER = sizeof.uint8_t,
}
& util.record.only_if _central_defines.REPEAT_KEY_ENABLE {
  REPEAT_KEY = sizeof.uint8_t * 4,
}
& util.record.only_if (led_count > 0) (
  {
    LED_BUFFER = sizeof.uint8_t * led_count * 3, # RGB
  }
) in

let _xaddr_defines =
  let names = std.record.fields _xaddr_sizes in
  let sizes = std.record.values _xaddr_sizes in
  sizes
  |> util.array.enumerate
  |> std.array.fold_left (fun acc { index, value } =>
      let is_dma_addr = std.string.is_match "^USB_EP\\d$" (std.array.at index names) in
      let start_must_even = kb.mcu.dma_must_even_address && is_dma_addr in

      let start =
        let s = acc.next in
        if (start_must_even && s % 2 != 0) then s + 1 else s in

      {
        starts = acc.starts @ [start],
        next = start + value,
      }
    ) { starts = [], next = 0 }
  |> (fun { starts, .. } => starts)
  |> util.array.enumerate
  |> std.array.map (fun { index, value } => { "XADDR_%{std.array.at index names}" = value })
  |> std.array.fold_left (&) {}
in

let _defines = {
  CH55X = match {
    'CH552 => 2,
    'CH559 => 9,
  } kb.mcu.family,
  F_CPU = match {
    'CH552 => std.to_string 24e6, # 24 MHz
    'CH559 => std.to_string 24e6, # 24 MHz
  } kb.mcu.family,
  SPLIT_ENABLE = side != 'self,
  SPLIT_SIDE_CENTRAL = side != 'peripheral,
  SPLIT_SIDE_PERIPHERAL = side == 'peripheral,

  UART0_ALT = std.array.elem "uart01" feature_keys_used,
  UART1_ALT = std.array.elem "uart11" feature_keys_used,
} & util.record.only_if (side != 'self) {
  SPLIT_PERIPH_KEY_COUNT =
    (if side == 'central then kb.split.peripheral else kb).keys
    |> std.array.length,
  SPLIT_PERIPH_ENCODER_COUNT =
    let expected_type = if side == 'central then 'peripheral else 'physical in
    kb.encoders
    |> std.array.filter (fun { type, .. } => type == expected_type)
    |> std.array.length,
} & util.record.only_if (soft_serial_pin >= 0) {
  SPLIT_SOFT_SERIAL_PIN =
    let p = soft_serial_pin in
    "P%{std.to_string (std.number.floor (p / 10))}.%{std.to_string (p % 10)}",
} & util.record.only_if (side != 'peripheral) (
  _central_defines & _xaddr_defines
)
in

let encode_hold_tap_behavior = fun b => {
  flags =
    (if b.timeout_decision == 'hold then 1 else 0)
    + (match { 'none => 0, 'hold => 2, 'tap => 4 } b.eager_decision)
    + (if b.global_quick_tap_ignore_consecutive then 8 else 0),
  timeout_ms = b.timeout_ms,
  key_interrupts = encode_hold_tap_key_interrupts b.key_interrupts,
} & util.record.only_if _defines.HOLD_TAP_QUICK_TAP_ENABLE {
  quick_tap_ms = b.quick_tap_ms
} & util.record.only_if _defines.HOLD_TAP_QUICK_TAP_INTERRUPT_ENABLE {
  quick_tap_interrupt_ms = b.quick_tap_interrupt_ms
} & util.record.only_if _defines.HOLD_TAP_GLOBAL_QUICK_TAP_ENABLE {
  global_quick_tap_ms = b.global_quick_tap_ms
} in

let encode_combo = fun combo key_idx => {
  timeout_ms = combo.timeout_ms,
  key_indices = combo.key_indices,
  key_idx_mapping = key_idx,
  flags =
    std.array.length combo.key_indices - 2
    + util.bit.shift (if combo.slow_release then 1 else 0) 3,
} & util.record.only_if _defines.COMBO_REQUIRE_PRIOR_IDLE_MS_ENABLE {
  require_prior_idle_ms = combo.require_prior_idle_ms,
} in

let Kscan = std.contract.from_predicate (fun k =>
  let all_used_pins = k.ins @ k.cols @ k.rows @ encoder_pins_used @ feature_pins_used
    # Include soft serial pin
    @ (if soft_serial_pin >= 0 then [soft_serial_pin] else []) in
  (util.array.unique all_used_pins) == all_used_pins
) in

let rec _kscan | Kscan = 
  let index_of_in = fun pin => util.array.index_of pin _kscan.ins in
  let index_of_col = fun pin => util.array.index_of pin _kscan.cols in
  let index_of_row = fun pin => util.array.index_of pin _kscan.rows in
{
  ins = kb.keys
    |> std.array.filter (fun k => k.type == 'direct)
    |> std.array.map (fun k => k.data),
  cols = kb.matrix.cols,
  rows = kb.matrix.rows,
  matrix = {
    cols = std.array.map index_of_col kb.matrix.cols,
    rows = std.array.map index_of_row kb.matrix.rows,
    mapping =
      let gen_mapping = fun direction =>
        let force_check = std.array.any (fun k =>
          k.type == 'matrix 
          && k.data.col >= 0 
          && k.data.row >= 0
        ) kb.keys in

        let ins =  if direction == 'col_to_row then cols else rows in
        let outs = if direction == 'col_to_row then rows else cols in

        if force_check then (
          std.array.generate (
            fun _out => std.array.generate (
              fun _in =>
                let k = {
                  type = 'matrix,
                  data.col = if direction == 'col_to_row then _in else _out,
                  data.row = if direction == 'col_to_row then _out else _in,
                  data.direction = direction,
                } in
                if std.array.elem k kb.keys then
                  util.array.index_of k kb.keys
                else
                  -1
            ) (std.array.length ins)
          ) (std.array.length outs)
        ) else []
      in
      {
        col_to_row = gen_mapping 'col_to_row,
        row_to_col = gen_mapping 'row_to_col,
      } 
  },
  direct = kb.keys
    |> util.array.enumerate
    |> std.array.filter (fun { index, value } => value.type == 'direct)
    |> std.array.map (fun { index, value } => {
        in_idx = index_of_in value.data,
        key_idx = index,
      }),
} in

{
  defines = _defines,
  kscan = _kscan,
  "%{"side"}" = side,

  key_map = if side == 'peripheral then [] else
    std.array.map (fun layer => std.array.map encode_kc layer) km.layers,

  hold_tap_behaviors = if side == 'peripheral then [] else
    std.array.map encode_hold_tap_behavior _hold_tap_behaviors,

  tap_dance_bindings = if side == 'peripheral then [] else
    std.array.map encode_kc (std.array.flatten _tap_dance_bindings),
  
  combo_defs = if side == 'peripheral then [] else
    std.array.map (fun c => encode_combo
      c.data
      (virtual_key_idx_start + util.array.index_of c km.virtual_keys)
    ) combos,
  
  encoder_defs = kb.encoders
    |> util.array.enumerate
    |> std.array.map (fun { index, value } => 
        let type = value.type in
        let data = value.data in
        let find_key_idx = fun dir =>
          let vk = { type = 'encoder, data = { "%{"index"}" = index, direction = dir } } in
          let i = util.array.index_of_or_n1 vk km.virtual_keys in
          if i < 0 then 0 else (virtual_key_idx_start + i)
        in
        {
          resolution = (if type == 'peripheral then (std.array.at data kb.split.peripheral.encoders).data else data).resolution,
          key_idx_cw = find_key_idx 'cw,
          key_idx_ccw = find_key_idx 'ccw,
        } & util.record.only_if (type == 'physical) {
          pin_a = data.pin_a,
          pin_b = data.pin_b,
        }
    ),
  
  led_defs = kb.leds
    |> util.array.enumerate,
#    |> std.array.map_with_index (fun i led => {
#      index = i,
#      value = { # Simply the pin number for now.
#        pin = match { 
#          'physical => led.data.pin.data,
#          'ws2812b => led.data.pin.data,
#        } led.type,
#        num = led.data.num,
#      }
#    }),
  
  led_map = km.leds, # no additional encoding necessary

  macro_steps = if side =='peripheral then [] else _macro_steps,
  macro_step_args = if side =='peripheral then [] else _macro_step_args,

  conditional_layers = if side == 'peripheral then [] else (
    km.conditional_layers
    |> std.record.to_array
    |> std.array.map (fun { field, value } => {
      then_layer = std.string.to_number field,
      if_layers = value
        |> std.array.map (fun i => util.bit.shift 1 i)
        |> std.array.fold_left (+) 0,
    })
  ),

  split_periph_key_indices = 
    if side != 'central then [] else
      kb.keys
      |> util.array.enumerate
      |> std.array.filter (fun { index, value } => value.type == 'peripheral)
      |> std.array.map (fun { index, value } => index),

  split_periph_encoder_indices =
    if side != 'central then [] else
      kb.encoders
      |> util.array.enumerate
      |> std.array.filter (fun { index, value } => value.type == 'peripheral)
      |> std.array.map (fun { index, value } => index),
}
