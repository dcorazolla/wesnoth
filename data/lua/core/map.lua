--[========[Map module]========]
print("Loading map module...")

function wesnoth.map.split_terrain_code(code)
	return table.unpack(code:split('^', {remove_empty = false}))
end

function wesnoth.map.read_location(...)
	local x, y = ...
	if y == nil then
		if type(x.x) == 'number' and type(x.y) == 'number' then
			x, y = x.x, x.y
		elseif type(x[1]) == 'number' and type(x[2]) == 'number' then
			x, y = table.unpack(x)
		else
			return nil, 0
		end
		return {x = x, y = y}, 1
	elseif type(x) == 'number' or type(y) == 'number' then
		return {x = x, y = y}, 2
	end
	return nil, 0
end

if wesnoth.kernel_type() ~= "Application Lua Kernel" then
	-- possible terrain string inputs:
	-- A        A^       A^B      ^        ^B
	-- implied mode:
	-- both     base     both     overlay  overlay
	function wesnoth.map.replace_base(code)
		local base, overlay = wesnoth.map.split_terrain_code(code)
		if base == nil then -- ^ or ^B
			-- There's no base to replace with, so do nothing
			return ''
		else
			-- Use the specified base but ignore the overlay
			return base .. '^'
		end
	end
	
	function wesnoth.map.replace_overlay(code)
		local base, overlay = wesnoth.map.split_terrain_code(code)
		if overlay == nil or overlay == '' then -- A or A^
			-- No overlay was specified, so we want to clear the overlay without touching the base
			return '^'
		else
			-- An overlay was specified, so use that and ignore the base
			return '^' .. overlay
		end
	end
	
	function wesnoth.map.replace_both(code)
		local base, overlay = wesnoth.map.split_terrain_code(code)
		if base == '' then -- ^ or ^B
			-- There's no way to find a base to replace with in this case.
			-- Could use the existing base, but that's not really replacing both, is it?
			error('replace_both: no base terrain specified')
		elseif overlay == '' then -- A^
			-- This would normally mean replace base while preserving overlay,
			-- but we actually want to replace base and clear overlay.
			return base
		else
			-- It's already going to replace both, so return unchanged
			return code
		end
	end
end

if wesnoth.kernel_type() == "Game Lua Kernel" then
	wesnoth.terrain_mask = wesnoth.deprecate_api('wesnoth.terrain_mask', 'wesnoth.current.map:terrain_mask', 1, nil, function(...)
		wesnoth.current.map:terrain_mask(...)
	end)
	wesnoth.get_terrain = wesnoth.deprecate_api('wesnoth.get_terrain', 'wesnoth.current.map[loc]', 1, nil, function(x, y)
		local loc = wesnoth.read_location(x, y)
		if loc == nil then error('get_terrain: expected location') end
		return wesnoth.current.map[loc]
	end)
	wesnoth.set_terrain = wesnoth.deprecate_api('wesnoth.set_terrain', 'wesnoth.current.map[loc]=', 1, nil, function(...)
		local loc, n = wesnoth.read_location(...)
		if n == 0 then error('set_terrain: expected location') end
		local new_ter, mode, replace_if_failed = select(n + 1, ...)
		if new_ter == '' or type(new_ter) ~= 'string' then error('set_terrain: expected terrain string') end
		if replace_if_failed then
			mode = mode or 'both'
			new_ter = wesnoth.map.replace_if_failed(new_ter, mode, true)
		elseif mode == 'both' or mode == 'base' or mode == 'overlay' then
			new_ter = wesnoth.map['replace_' .. mode](new_ter)
		else
			error('set_terrain: invalid mode')
		end
		wesnoth.current.map[loc] = new_ter
	end)
	wesnoth.get_map_size = wesnoth.deprecate_api('wesnoth.get_map_size', 'wesnoth.current.map.playable_width,playable_height,border_size', 1, nil, function()
		local m = wesnoth.current.map
		return m.playable_width, m.playable_height, m.border_size
	end)
	wesnoth.special_locations = wesnoth.deprecate_api('wesnoth.special_locations', 'wesnoth.current.map:special_locations', 1, nil, setmetatable({}, {
		__index = function(_, k) return wesnoth.current.map.special_locations[k] end,
		__newindex = function(_, k, v) wesnoth.current.map.special_locations[k] = v end,
		__len = function(_)
			local n = 0
			for k,v in pairs(wesnoth.current.map.special_locations) do
				n = n + 1
			end
			return n
		end,
		__pairs = function(_) return pairs(wesnoth.current.map.special_locations) end,
	}), 'Note: the length operator has been removed')
	
	wesnoth.place_shroud = wesnoth.deprecate_api('wesnoth.place_shroud', 'wesnoth.map.place_shroud', 1, nil, wesnoth.map.place_shroud)
	wesnoth.remove_shroud = wesnoth.deprecate_api('wesnoth.remove_shroud', 'wesnoth.map.remove_shroud', 1, nil, wesnoth.map.remove_shroud)
	wesnoth.is_shrouded = wesnoth.deprecate_api('wesnoth.is_shrouded', 'wesnoth.map.is_shrouded', 1, nil, wesnoth.map.is_shrouded)
	wesnoth.add_fog = wesnoth.deprecate_api('wesnoth.add_fog', 'wesnoth.map.place_fog', 1, nil, wesnoth.map.place_fog)
	wesnoth.remove_fog = wesnoth.deprecate_api('wesnoth.remove_fog', 'wesnoth.map.remove_fog', 1, nil, wesnoth.map.remove_fog)
	wesnoth.is_fogged = wesnoth.deprecate_api('wesnoth.is_fogged', 'wesnoth.map.is_fogged', 1, nil, wesnoth.map.is_fogged)
	wesnoth.get_village_owner = wesnoth.deprecate_api('wesnoth.get_village_owner', 'wesnoth.map.get_owner', 1, nil, wesnoth.map.get_owner)
	wesnoth.set_village_owner = wesnoth.deprecate_api('wesnoth.set_village_owner', 'wesnoth.map.set_owner', 1, nil, wesnoth.map.set_owner)
	wesnoth.label = wesnoth.deprecate_api('wesnoth.label', 'wesnoth.map.add_label', 1, nil, wesnoth.map.add_label)
	wesnoth.add_time_area = wesnoth.deprecate_api('wesnoth.add_time_area', 'wesnoth.map.place_area', 1, nil, wesnoth.map.place_area)
	wesnoth.remove_time_area = wesnoth.deprecate_api('wesnoth.remove_time_area', 'wesnoth.map.remove_area', 1, nil, wesnoth.map.remove_area)
	wesnoth.get_locations = wesnoth.deprecate_api('wesnoth.get_locations', 'wesnoth.map.find', 1, nil, wesnoth.map.find)
	wesnoth.get_villages = wesnoth.deprecate_api('wesnoth.get_locations', 'wesnoth.map.find', 1, nil, function(cfg)
		return wesnoth.map.find{gives_income = true, wml.tag["and"](cfg)}
	end)
	wesnoth.match_location = wesnoth.deprecate_api('wesnoth.match_location', 'wesnoth.map.matches', 1, nil, wesnoth.map.matches)
end