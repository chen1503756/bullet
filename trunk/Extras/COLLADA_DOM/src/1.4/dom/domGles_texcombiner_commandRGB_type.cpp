/*
 * Copyright 2006 Sony Computer Entertainment Inc.
 *
 * Licensed under the SCEA Shared Source License, Version 1.0 (the "License"); you may not use this 
 * file except in compliance with the License. You may obtain a copy of the License at:
 * http://research.scea.com/scea_shared_source_license.html
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License 
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
 * implied. See the License for the specific language governing permissions and limitations under the 
 * License. 
 */

#include <dae/daeDom.h>
#include <dom/domGles_texcombiner_commandRGB_type.h>

daeElementRef
domGles_texcombiner_commandRGB_type::create(daeInt bytes)
{
	domGles_texcombiner_commandRGB_typeRef ref = new(bytes) domGles_texcombiner_commandRGB_type;
	return ref;
}


daeMetaElement *
domGles_texcombiner_commandRGB_type::registerElement()
{
    if ( _Meta != NULL ) return _Meta;
    
    _Meta = new daeMetaElement;
    _Meta->setName( "gles_texcombiner_commandRGB_type" );
	_Meta->setStaticPointerAddress(&domGles_texcombiner_commandRGB_type::_Meta);
	_Meta->registerConstructor(domGles_texcombiner_commandRGB_type::create);

	// Add elements: argument
    _Meta->appendArrayElement(domGles_texcombiner_argumentRGB_type::registerElement(),daeOffsetOf(domGles_texcombiner_commandRGB_type,elemArgument_array),"argument"); 

	//	Add attribute: operator
 	{
		daeMetaAttribute *ma = new daeMetaAttribute;
		ma->setName( "operator" );
		ma->setType( daeAtomicType::get("Gles_texcombiner_operatorRGB_enums"));
		ma->setOffset( daeOffsetOf( domGles_texcombiner_commandRGB_type , attrOperator ));
		ma->setContainer( _Meta );
	
		_Meta->appendAttribute(ma);
	}

	//	Add attribute: scale
 	{
		daeMetaAttribute *ma = new daeMetaAttribute;
		ma->setName( "scale" );
		ma->setType( daeAtomicType::get("xsFloat"));
		ma->setOffset( daeOffsetOf( domGles_texcombiner_commandRGB_type , attrScale ));
		ma->setContainer( _Meta );
		ma->setIsRequired( false );
	
		_Meta->appendAttribute(ma);
	}
	
	
	_Meta->setElementSize(sizeof(domGles_texcombiner_commandRGB_type));
	_Meta->validate();

	return _Meta;
}


daeMetaElement * domGles_texcombiner_commandRGB_type::_Meta = NULL;


