-- Copyright (c) 2014-2015:  G-CSC, Goethe University Frankfurt
-- Author: Martin Rupp
-- 
-- This file is part of UG4.
-- 
-- UG4 is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License version 3 (as published by the
-- Free Software Foundation) with the following additional attribution
-- requirements (according to LGPL/GPL v3 §7):
-- 
-- (1) The following notice must be displayed in the Appropriate Legal Notices
-- of covered and combined works: "Based on UG4 (www.ug4.org/license)".
-- 
-- (2) The following notice must be displayed at a prominent place in the
-- terminal output of covered works: "Based on UG4 (www.ug4.org/license)".
-- 
-- (3) The following bibliography is recommended for citation and must be
-- preserved in all covered files:
-- "Reiter, S., Vogel, A., Heppner, I., Rupp, M., and Wittum, G. A massively
--   parallel geometric multigrid solver on hierarchically distributed grids.
--   Computing and visualization in science 16, 4 (2013), 151-164"
-- "Vogel, A., Reiter, S., Rupp, M., Nägel, A., and Wittum, G. UG4 -- a novel
--   flexible software system for simulating pde based models on high performance
--   computers. Computing and visualization in science 16, 4 (2013), 165-179"
-- 
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
-- GNU Lesser General Public License for more details.

util = util or {}
--------------------------------------------------------------------------------
-- Command line functions
--------------------------------------------------------------------------------

--! @return name, distance of the index of the table 'list' which is most equal to str
function GetMinLevenshteinDistanceOfIndex(str, list)
	local imin=10
	local namemin=""
	for i,v in pairs(list) do
		local d = LevenshteinDistance(str, i)
		if d < imin then
			imin = d
			namemin = i
		end
	end
	return namemin, imin
end

--! @return name, distance of the value of the table 'list' which is most equal to str
function GetMinLevenshteinDistanceOfValue(str, list)
	local imin=10
	local namemin=""
	for i,v in pairs(list) do
		local d = LevenshteinDistance(str, v)
		if d < imin then
			imin = d
			namemin = v
		end
	end
	return namemin, imin
end

function util.ConcatOptions(options)
	local sOpt = ""
	if options ~= nil then
		sOpt = " ["
		for i=1,#options do
			if i > 1 then sOpt = sOpt.." | " end
			sOpt = sOpt..options[i]
		end
		sOpt = sOpt.."]"
	end
	return sOpt
end

function util.CheckOptionsType(name, options, atype)
   if options ~= nil then
		for i=1,#options do
			ug_assert(type(options[i]) == atype, "ERROR in util.GetParam: passed option '"..options[i]..
		    			"' for '"..name.."' not a "..atype)
		end
	end
end

function util.CheckOptionsValue(name, value, options)
	if options ~= nil then
		local bValid = false
		for i=1,#options do
			if value == options[i] then bValid = true; end
		end
		local s = ""
		local minname, imin = GetMinLevenshteinDistanceOfValue(value, options)
		if imin < 5 then s = "\nDid you mean '"..minname.."' ?" end
		ug_assert(bValid, "ERROR in util.GetParam: passed value '"..value.."' for '"
				..name.."' not contained in options:"..util.ConcatOptions(options)..s)
	end
end

util.args = util.args or {}
util.argsUsed = util.argsUsed or {}

--! util.GetParam
--! returns parameter in ugargv after ugargv[i] == name
--! @param name parameter in ugargv to search for
--! @param default returned value if 'name' is not present (default nil)
--! @param description description for 'name' (default nil)
--! @param options a table of options e.g. {"jac", "sgs"}
--! @param atype type of the parameter, e.g. "number", "string", "boolean". default "string"
--! @return parameter in ugargv after ugargv[i] == name or default if 'name' was not present
function util.GetParam(name, default, description, options, atype)
	-- check options
    if options ~= nil then
	    ug_assert(type(options) == "table",
	    	"ERROR in util.GetParam: passed options for '"..name.."' not a table.")
	    
	    if atype == nil then
	    	util.CheckOptionsType(name, options, "string")
	    end
    end

	-- store infos
	util.args[name] = {}
	util.args[name].description = (description or "")
	util.args[name].default = default	
	util.args[name].type = (atype or " (string) ")
	util.args[name].options = options

	-- check if argument passed
	local value = default
	local bFound = false
	for i = 2, ugargc-1 do
		if not(bFound) and ugargv[i] == name then			
			util.argsUsed[i]=true
			util.argsUsed[i+1]=true
			value = ugargv[i+1]
			bFound=true
		end
	end
	
	local iFound=0
	for i = 2, ugargc do
		if ugargv[i] == name then
			iFound=iFound+1			
		end
	end
	if iFound > 1 then		
		ug_warning("--- WARNING: Parameter "..name.." found multiple times ---")
		if util.bFailOnMultipleParameters == true then
			exit()
		end
	end
	util.args[name].value = value
	
	-- check options
	if atype == nil then
		util.CheckOptionsValue(name, value, options)
	end
	
	-- return default
	return value; 
end
 

--! util.GetParamNumber
--! use with CommandLine to get a number, like -numRefs 4
--! if parameter is not specified, returns default
--! @param name parameter in ugargv to search for
--! @param default returned value if 'name' is not present (default nil)
--! @param description description for 'name' (default nil)
--! @return the number after the parameter 'name' or default if 'name' was not present/no number
function util.GetParamNumber(name, default, description, options)

	-- check options
	util.CheckOptionsType(name, options, "number")
	
	-- read in
	local param = util.GetParam(name, default, description, options, " (number) ")
	if param == nil and default == nil then
		return nil
	end

	ug_assert(param ~= nil, "ERROR in GetParamNumber: Number Parameter "..name.." not set and no default value given.")
	if param == default then
		return default
	end
	
	-- cast to number	
	local value = tonumber(param)
	ug_assert(value ~= nil, "ERROR in GetParamNumber: passed '"..param.."' for Parameter '"
				..name.."' is not a number.")

	-- check options
	util.CheckOptionsValue(name, value, options)			
	
	-- return value
	return value
end

--! util.GetParamNumber
--! use with CommandLine to get an array of numbers, like -proc "1;4;9"
--! if parameter is not specified, returns default
--! @param name parameter in ugargv to search for
--! @param default returned value if 'name' is not present (default nil)
--! @param description description for 'name' (default nil)
--! @return the number after the parameter 'name' or default if 'name' was not present/no number
function util.GetParamNumberArray(name, default, description)
	
	-- read in
	local param = util.GetParam(name, default, description)
	if param == nil and default == nil then
		return nil
	end

	ug_assert(param ~= nil, "ERROR in GetParamNumberArray: Parameter "..name.." not set and no default value given.")
	if param == default then
		return default
	end
	
	local array = {}
	local i = 1
	for str in string.gmatch(param, "([^;]+)") do
		local value = tonumber(str)
		ug_assert(value ~= nil, "ERROR in GetParamNumberArray: passed '"..param.."' for Parameter '"
			..name.."' is not an array of numbers.")
		array[i] = value
		i = i + 1
	end
	
	return array
end

--! util.GetParamBool
--! use with CommandLine to get boolean option, like -useAMG true
--! @param default returned value if 'name' is not present (default nil)
--! @param description description for 'name' (default nil)
--! @return the number after the parameter 'name' or default if 'name' was not present/no number
--! unlike util.HasParamOption , you must specify a value for your optionn, like -useAMG 1
--! possible values are (false): 0, n, false, (true) 1, y, j, true
function util.GetParamBool(name, default, description)

	local r = util.GetParam(name, tostring(default), description, nil, "  (bool)  ")
	
	if r == "0" or r == "false" or r == "n" then
		return false
	elseif r == "1" or r == "true" or r == "y" or r == "j" then
		return true
	else
		print("ERROR in GetParamBool: passed '"..r.."' for Parameter '"..name.."' is not a bool.")
		exit();
	end
end

--! util.GetParamFromList
--! use with CommandLine to get a value out of a list
--! @param name name of the option, like -smoother
--! @param default returned value if 'name' is not present (default nil)
--! @param description description for 'name' (default nil)
--! @return the number after the parameter 'name' or default if 'name' was not present/no number
--! unlike util.HasParamOption , you must specify a value for your optionn, like -useAMG 1
--! possible values are (false): 0, n, false, (true) 1, y, j, true
function util.GetParamFromList(name, default, list)
	local n = util.GetParam(name, default)
	if list[n] == nil then
		print("\n\noption \""..n.."\" not supported")
		local s = "available options: "
		local first = true
		for i, v in pairs(list) do
			if first then first = false else s = s..", " end
			s = s..i			
		end
		local namemin, imin = GetMinLevenshteinDistanceOfIndex(n, list)
		if imin < 5 then
			s = s.."\nDid you mean '"..namemin.."' ?"
		end
		
		print(s)
		
		ug_assert(false, "option \""..n.."\" not supported\n"..s)
	else
		return list[n]
	end	
end


--! util.HasParamOption
--! use with CommandLine to get option, like -useAMG
--! @param name option in argv to search for
--! @param description description for 'name' (default nil)
--! @return true if option found, else false
function util.HasParamOption(name, description)

	-- store infos
	util.args[name] = {}
	util.args[name].description = (description or "")
	util.args[name].default = "false"	
	util.args[name].type = " [option] "
	util.args[name].value = "false"

	-- check if passed
	for i = 1, ugargc do
		if ugargv[i] == name then
			util.argsUsed[i]=true
			util.args[name].value = "true"
			return true
		end		
	end	
	
	-- not passed
	return false 
end

--! util.CheckForParam
--! Checks whether a paramneter was specified in the command line
--! @param name of the parameter to search for in argv
--! @return true if the parameter was found, false if not
function util.CheckForParam(name)
	for i = 1, ugargc do
		if ugargv[i] == name then
			return true
		end		
	end	
	
	return false 
end


--! returns all arguments from the command line
function util.GetCommandLine()
	local pline = ""
	for i=1, ugargc do
		pline = pline..ugargv[i].." "
	end
	return pline
end

--! lists all the command line arguments which where used or could
--! have been used with util.GetParam, util.GetParamNumber and util.HasParamOption
function util.PrintArguments()
	local length=0
	for name,arg in pairsSortedByKeys(util.args) do
		length = math.max(string.len(name), length)
	end	
	for name,arg in pairsSortedByKeys(util.args) do
		print("  "..util.adjuststring(name, length, "l").." = "..arg.value)
	end
end

function StrOrNil(s)
	if s == nil then return "nil" end
	return s
end

--! prints out the description to each GetParam-parameter so far called
function util.PrintHelp()
	local length=0
	for name,arg in pairsSortedByKeys(util.args) do
		length = math.max(string.len(name), length)
	end	
	local defaultStr
	for name,arg in pairsSortedByKeys(util.args) do
		sOpt = util.ConcatOptions(arg.options)
		print(arg.type..util.adjuststring(name, length, "l").." = "..StrOrNil(arg.value)..
			  " : "..arg.description..sOpt.." (default = "..StrOrNil(arg.default)..")")
	end
end

--! calls util.PrintHelp() and exits if HasParamOption("-help")
--! @param optional description of the script
function util.CheckAndPrintHelp(desc)
	if util.HasParamOption("-help", "print this help") then
		if desc ~= nil then print(desc) end
		print()			
		util.PrintHelp()
		exit()
	end
end

--! lists all command line arguments which were provided but could not be used.
function util.PrintIgnoredArguments()
	if bPrintIgnoredArgumentsCalled then return end
	bPrintIgnoredArgumentsCalled = true
	local pline = ""
	for i=2, ugargc do
		if ugargv[i] == "-ex"
		 	or ugargv[i] == "-outproc"
			or ugargv[i] == "-outproc"
			or ugargv[i] == "-logtofile" then
			ugargc=ugargc+1
		elseif
			ugargv[i] == "-noterm"
			or ugargv[i] == "-noquit" then
			-- continue			
				
		elseif (util.argsUsed == nil or util.argsUsed[i] == nil) and
			string.sub(ugargv[i], 1,1) == "-" 
			 then
			local imin=10
			local namemin=""
			for name,arg in pairs(util.args) do
				if name == ugargv[i] then
					imin = 0
				else
					local d = LevenshteinDistance(name, ugargv[i])
					if d < imin then
						imin = d
						namemin = name
					end
				end
			end
			if imin == 0 then
				pline=pline..ugargv[i].." [specified multiple times. Used value: "..util.args[ugargv[i]].value.."]\n"
			elseif imin < string.len(ugargv[i])/2 then
				pline=pline..ugargv[i].." [did you mean "..namemin..util.args[namemin].type.."?]\n"
			else
				pline=pline..ugargv[i].." "				
			end
		end
	end
	if pline ~= "" then
		print("WARNING: Ignored arguments (or not parsed with util.GetParam/util.GetParamNumber/util.HasParamOption) :\n"..pline.."\n")
	end
end


function util.GetUniqueFilenameFromCommandLine()
	local ret=""
	for i = 1, ugargc do
		ret = ret.." "..ugargv[i]		
	end
	ret = FilenameStringEscape(ret)
	if NumProcs() > 1 then
		return ret.."_numProcs_"..NumProcs()
	else
		return ret
	end
end

util.HasParamOption("-noquit", "Runs the interactive shell after specified script")
util.HasParamOption("-noterm", "Terminal logging will be disabled")
util.HasParamOption("-profile", "Shows profile-output when the application terminates")
util.GetParamNumber("-outproc", 0, "Sets the output-proc to id.")
util.GetParam("-ex", "", "Executes the specified script")
util.GetParam("-logtofile", "", "Output will be written to the specified file")
