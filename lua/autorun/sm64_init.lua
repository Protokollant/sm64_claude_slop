SM64 = SM64 or {}
SM64.SCALE    = 0.45
SM64.ROM_PATH = "sm64/baserom.us.z64"

if SERVER then
	AddCSLuaFile("sm64/cl_surfaces.lua")
	AddCSLuaFile("sm64/cl_sm64.lua")
	include("sm64/sv_sm64.lua")
else
	include("sm64/cl_surfaces.lua")
	include("sm64/cl_sm64.lua")
end
