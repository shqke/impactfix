Description
------
SourceMod extension that improves impact detection while charging against physics objects with a mass below 250.

Video Demonstration
------
[![Charger impact against immovable props PoC](http://img.youtube.com/vi/TJ6Twi9qg68/default.jpg)](https://www.youtube.com/watch?v=TJ6Twi9qg68 "Charger impact against immovable props PoC")

Requirements
------
- [MM:Source (1.10+)](https://www.sourcemm.net/)
- [SourceMod (1.8+)](https://www.sourcemod.net/)

Supported Games
------
- [Left 4 Dead 2](https://store.steampowered.com/app/550/Left_4_Dead_2/)

Installation
------
1. Get [latest impactfix release](https://github.com/shqke/impactfix/actions) for your OS (linux or windows)
2. Extract the zip file into your server's mod folder

Test Cases
------
#### c2m2 custom immovable prop dynamic -> stop (Asleep = true)
```
Hit: 1719 ("prop_dynamic")
  Movetype: 7
  Motion Enabled: true
  Static: true
  Asleep: true
  IsAlive: true
  Model: "models/props_misc/fairground_tent_closed.mdl"
```
---
#### c1m1 death charge glass (lower floor) -> continue (IsAlive = false)
```
Hit: 227 ("func_breakable")
  Movetype: 7
  Motion Enabled: true
  Static: true
  Asleep: true
  IsAlive: true -> false
  Model: "*30"
```
------
#### c1m1 death charge glass (top floor) -> continue (IsAlive = false)
```
Hit: 233 ("prop_physics")
  Movetype: 6 -> 0
  Motion Enabled: false -> true
  Static: false
  Asleep: true -> false
  IsAlive: true -> false
  Model: "models/props_windows/hotel_window_glass001.mdl"
```
------
#### c10m4 breakable window -> continue (IsAlive = false)
```
Hit: 375 ("prop_physics")
  Movetype: 6 -> 0
  Motion Enabled: false -> true
  Static: false
  Asleep: true -> false
  IsAlive: true -> false
  Model: "models/props_windows/window_industrial.mdl"
```
------
#### c10m4 breakable wooden door near bus -> continue (IsAlive = false)
```
Hit: 414 ("func_breakable")
  Movetype: 7
  Motion Enabled: true
  Static: true
  Asleep: true
  IsAlive: true -> false
  Model: "*111"
```
------
#### c1m4 unbreakable window by charger -> stop (Asleep = true)
```
Hit: 159 ("func_breakable")
  Movetype: 7
  Motion Enabled: true
  Static: true
  Asleep: true
  IsAlive: true
  Model: "*91"
```
------
#### c2m2 hittables -> continue (Asleep = false)
```
Hit: 840 ("prop_physics")
  Movetype: 6
  Motion Enabled: true
  Static: false
  Asleep: true -> false
  IsAlive: true
  Model: "models/props_street/trashbin01.mdl"
```
------
#### c10m5 immovable physics prop -> stop (Asleep = true)
```
Hit: 261 ("prop_physics")
  Movetype: 6
  Motion Enabled: true
  Static: false
  Asleep: true
  IsAlive: true
  Model: "models/props_interiors/table_picnic.mdl"
```
------
#### c6m1 wedding chairs -> continue (Asleep = false)
```
Hit: 81 ("prop_physics")
  Movetype: 6
  Motion Enabled: false -> true
  Static: false
  Asleep: true -> false
  IsAlive: true
  Model: "models/props_urban/plastic_chair001_debris.mdl"
```
------
#### c8m5 minigun -> stop (Asleep = true)
```
Hit: 150 ("prop_minigun_l4d1")
  Movetype: 7
  Motion Enabled: true
  Static: true
  Asleep: true
  IsAlive: true
  Model: "models/w_models/weapons/w_minigun.mdl"
```
