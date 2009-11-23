# translations.cmake - Translations using APT's translation system.
# Copyright (C) 2009 Julian Andres Klode <jak@debian.org>
#
#

macro(apt_add_translation_domain domain files)
	# Create the template for this specific sub-domain
	add_custom_command (
		OUTPUT ${PROJECT_BINARY_DIR}/${domain}.pot
		COMMAND xgettext --add-comments --foreign -k_ -kN_
		                 -o ${PROJECT_BINARY_DIR}/${domain}.pot ${files}
	)
	
	file(GLOB translations "${PROJECT_SOURCE_DIR}/po/*.po")
	
	foreach(file ${translations})
		get_filename_component(langcode ${file} NAME_WE)
		set(outdir ${PROJECT_BINARY_DIR}/locale/${langcode}/LC_MESSAGES)
		file(MAKE_DIRECTORY ${outdir})
		
		# Command to merge and compile the messages
		add_custom_command(
			OUTPUT ${outdir}/${domain}.mo
			COMMAND msgmerge -qo - ${file} ${PROJECT_BINARY_DIR}/${domain}.pot |
					msgfmt -o ${outdir}/${domain}.mo -
			DEPENDS ${file} ${PROJECT_BINARY_DIR}/${domain}.pot
		)
		
		set(mofiles ${mofiles} ${outdir}/${domain}.mo)
		install(FILES ${outdir}/${domain}.mo DESTINATION "share/locale/${langcode}/LC_MESSAGES")
	endforeach(file ${translations})
	
	add_custom_target(nls-${domain} ALL DEPENDS ${mofiles})
	
	foreach(file ${files})
		file(RELATIVE_PATH relfile ${PROJECT_SOURCE_DIR} ${file})
		set(PROJECT_I18N ${PROJECT_I18N} ${relfile} PARENT_SCOPE)
	endforeach(file ${files})
endmacro(apt_add_translation_domain domain files)