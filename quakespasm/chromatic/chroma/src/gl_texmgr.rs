/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// gl_texmgr.rs -- fitzquake's texture manager. manages opengl texture images

use gl::types::*;
use gl_model::QModelT;
use std::os::raw::{c_char, c_int, c_schar, c_uint, c_ushort};
use std::ptr::null_mut;
use MAX_QPATH;

bitflags! {
    #[derive(Default)]
    #[repr(C)]
    pub struct TexPref: c_uint {
        /// generate mipmaps
        const Mipmap = 0x0001;
        /// Nearest and Linear aren't supposed to be ORed with Mipmap
        /// force linear
        const Linear = 0x0002;
        /// force nearest
        const Nearest = 0x0004;
        /// allow alpha
        const Alpha = 0x0008;
        /// allow padding
        const Pad = 0x0010;
        /// never free
        const Persist = 0x0020;
        /// overwrite existing same-name texture
        const Overwrite = 0x0040;
        /// always load full-sized
        const NoPicMip = 0x0080;
        /// use fullbright mask palette
        const FullBright = 0x0100;
        /// use nobright mask palette
        const NoBright = 0x0200;
        /// use conchars palette
        const Conchars = 0x0400;
        /// resize this texture when warpimagesize changes
        const WarpImage = 0x0800;
    }
}

#[derive(Clone, Copy, PartialEq)]
#[repr(C)]
pub enum SrcFormat {
    Indexed,
    LightMap,
    RGBA,
}

impl SrcFormat {
    pub const fn default() -> Self {
        SrcFormat::Indexed
    }
}

impl Default for SrcFormat {
    fn default() -> Self {
        Self::default()
    }
}

pub type SrcOffsetT = libc::uintptr_t;

#[derive(Clone, Copy)]
#[repr(C)]
pub struct GlTextureT {
    /// managed by texture manager
    texnum: GLuint,
    next: *mut GlTextureT,
    owner: *mut QModelT,
    /// managed by image loading
    name: [c_char; 64],
    /// width of image as it exists in opengl
    width: c_uint,
    /// height of image as it exists in opengl
    height: c_uint,
    flags: c_uint,
    /// relative filepath to data source, or "" if source is in memory
    source_file: [c_char; MAX_QPATH],
    /// byte offset into file, or memory address
    source_offset: SrcOffsetT,
    /// format of pixel data (indexed, lightmap, or rgba)
    source_format: SrcFormat,
    /// width of image in source data
    source_width: c_uint,
    /// height of image in source data
    source_height: c_uint,
    /// generated by source data before modifications
    source_crc: c_ushort,
    /// 0-13 shirt color, or -1 if never colormapped
    shirt: c_schar,
    /// 0-13 pants color, or -1 if never colormapped
    pants: c_schar,
    /// used for rendering; matches r_framecount if texture was bound this frame
    visframe: c_int,
}

impl GlTextureT {
    pub const fn default() -> Self {
        Self {
            texnum: 0,
            next: null_mut(),
            owner: null_mut(),
            name: [0; 64],
            width: 0,
            height: 0,
            flags: 0,
            source_file: [0; MAX_QPATH],
            source_offset: 0,
            source_format: SrcFormat::default(),
            source_width: 0,
            source_height: 0,
            source_crc: 0,
            shirt: 0,
            pants: 0,
            visframe: 0,
        }
    }
}

impl Default for GlTextureT {
    fn default() -> Self {
        Self::default()
    }
}

pub mod capi {}
