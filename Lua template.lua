--[[
This template is developed for my team when working with the Amazon lumberyard engine.
Inside this template i have premade some settings that can be uncommented to be used.
]]--

MyLuaEntity = 
{
	--[[local variables
		Myint 		= 1,
		MyFloat		= 1.0,
		Mystring	= "",
		Mybool		= 0,
	]]--
	Properties
	{
		--[[editor variables
		--http://docs.aws.amazon.com/lumberyard/latest/developerguide/entity-property-prefixes.html
		iMyint 			= 1,
		fMyFloat		= 1.0,
		sMystring		= "",
		bMybool			= 0,
		object_MyModel	= "",
			]]--						
		--[[
		Physics = {
			--bPhysicalize = 1, -- True if object should be physicalized at all.
			--bRigidBody = 1, -- True if rigid body, False if static.
			--bPushableByPlayers = 1,
			--Density = -1,
			--Mass = -1,
		},			
		]]--
	},
	Editor
	{
		--Model = "",
		--ShowBounds = 0,
		--Icon = "",
		--IconOnTop = 0,	
	},
},

----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
------------ CALLBACKS ---------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
--[[
for all callbacks see:
http://docs.aws.amazon.com/lumberyard/latest/developerguide/callback-ref-entity-system-script.html
]]--


--[[
-- Called after an entity is created by the Entity system.
function MyLuaEntity:OnSpawn()

	--your code here


   self:OnReset(); 
   
end
]]--

--[[
-- Called when an entity gets initialized via ENTITY_EVENT_INIT, and when its ScriptProxy gets initialized.
function MyLuaEntity:OnSpawn()

	--your code here


   self:OnReset(); 
   

end
]]--

--[[ called when the game starts
function MyLuaEntity:OnStartGame()
	-- enable the following to register the entity in the updateloop, do it here if you dont want to update when in edit mode
    --self:Activate(1);
end

]]--
--[[
--your update loop, make sure self:Activate(1); is called in an function or else the update function won't be called
function MyLuaEntity:OnUpdate(dt)
	--your code here
end
]]--

--[[
-- called when an enemy is destroyed ingame
function OnDestroy()
	--your code here
end
]]--

--[[
-- called when you close the level
function MyLuaEntity:OnShutDown()
	--your code here
end
]]--

--[[
--Called by Lumberyard Editor when the user changes one of the properties.
function MyLuaEntity:OnPropertyChange()
	self:OnReset();
	--your code here
end
]]--

--[[
--Usually called when an editor wants to reset the state.
function MyLuaEntity:OnReset()
	--your code here
	
	self:Activate(0);
	--[[
	--reset the model
	if(self.properties.object_MyModel ~= "") then
		self:LoadObject(0,self.Properties.object_MyModel);
	end 
	]]--
	
--[[
	--add collision // dont forget to enable the function : PhysicalizeThis
	if(bPhysicalize == true) then
		self:PhysicalizeThis();
	end
	
	]]--
end

]]--

----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
------------ My functions ---------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
--[[
--add collision to the entity
function MyLuaEntity:PhysicalizeThis()
	EntityCommon.PhysicalizeRigid(self,0,self.Properties.Physics, 1);
end
]]--