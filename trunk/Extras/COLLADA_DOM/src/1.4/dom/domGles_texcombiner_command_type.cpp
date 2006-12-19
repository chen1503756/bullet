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
#include <dom/domGles_texcombiner_command_type.h>
#include <dae/daeMetaCMPolicy.h>
#include <dae/daeMetaSequence.h>
#include <dae/daeMetaChoice.h>
#include <dae/daeMetaGroup.h>
#include <dae/daeMetaAny.h>
#include <dae/daeMetaElementAttribute.h>

daeElementRef
domGles_texcombiner_command_type::create(daeInt bytes)
{
	domGles_texcombiner_command_typeRef ref = new(bytes) domGles_texcombiner_command_type;
	return ref;
}


daeMetaElement *
domGles_texcombiner_command_type::registerElement()
{
    if ( _Meta != NULL ) return _Meta;
    
    _Meta = new daeMetaElement;
    _Meta->setName( "gles_texcombiner_command_type" );
	_Meta->registerClass(domGles_texcombiner_command_type::create, &_Meta);

	daeMetaCMPolicy *cm = NULL;
	daeMetaElementAttribute *mea = NULL;
	cm = new daeMetaSequence( _Meta, cm, 0, 1, 1 );

	mea = new daeMetaElementAttribute( _Meta, cm, 0, 0, 1 );
	mea->setName( "constant" );
	mea->setOffset( daeOffsetOf(domGles_texcombiner_command_type,elemConstant) );
	mea->setElementType( domGles_texture_constant_type::registerElement() );
	cm->appendChild( mea );
	
	mea = new daeMetaElementAttribute( _Meta, cm, 1, 0, 1 );
	mea->setName( "RGB" );
	mea->setOffset( daeOffsetOf(domGles_texcombiner_command_type,elemRGB) );
	mea->setElementType( domGles_texcombiner_commandRGB_type::registerElement() );
	cm->appendChild( mea );
	
	mea = new daeMetaElementAttribute( _Meta, cm, 2, 0, 1 );
	mea->setName( "alpha" );
	mea->setOffset( daeOffsetOf(domGles_texcombiner_command_type,elemAlpha) );
	mea->setElementType( domGles_texcombiner_commandAlpha_type::registerElement() );
	cm->appendChild( mea );
	
	cm->setMaxOrdinal( 2 );
	_Meta->setCMRoot( cm );	
	
	
	_Meta->setElementSize(sizeof(domGles_texcombiner_command_type));
	_Meta->validate();

	return _Meta;
}


daeMetaElement * domGles_texcombiner_command_type::_Meta = NULL;


