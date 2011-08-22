/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2006 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

//$Id$

//mapnik
#include <mapnik/text_symbolizer.hpp>
// boost
#include <boost/scoped_ptr.hpp>
#include <mapnik/text_processing.hpp>

namespace mapnik
{

static const char * label_placement_strings[] = {
    "point",
    "line",
    "vertex",
    "interior",
    ""
};


IMPLEMENT_ENUM( label_placement_e, label_placement_strings )

static const char * vertical_alignment_strings[] = {
    "top",
    "middle",
    "bottom",
    "auto",
    ""
};


IMPLEMENT_ENUM( vertical_alignment_e, vertical_alignment_strings )

static const char * horizontal_alignment_strings[] = {
    "left",
    "middle",
    "right",
    "auto",
    ""
};


IMPLEMENT_ENUM( horizontal_alignment_e, horizontal_alignment_strings )

static const char * justify_alignment_strings[] = {
    "left",
    "middle",
    "right",
    ""
};


IMPLEMENT_ENUM( justify_alignment_e, justify_alignment_strings )

static const char * text_transform_strings[] = {
    "none",
    "uppercase",
    "lowercase",
    "capitalize",
    ""
};


IMPLEMENT_ENUM( text_transform_e, text_transform_strings )


text_symbolizer::text_symbolizer(text_placements_ptr placements)
    : symbolizer_base(),
      placement_options_(placements)
{

}

text_symbolizer::text_symbolizer(expression_ptr name, std::string const& face_name,
                                 unsigned size, color const& fill,
                                 text_placements_ptr placements)
    : symbolizer_base(),
      placement_options_(placements)
{
    set_name(name);
    set_face_name(face_name);
    set_text_size(size);
    set_fill(fill);
}

text_symbolizer::text_symbolizer(expression_ptr name, unsigned size, color const& fill,
                                 text_placements_ptr placements)
    : symbolizer_base(), placement_options_(placements)
{
    set_name(name);
    set_text_size(size);
    set_fill(fill);
}

text_symbolizer::text_symbolizer(text_symbolizer const& rhs)
    : symbolizer_base(rhs),
      placement_options_(rhs.placement_options_) /*TODO: Copy options! */ {}

text_symbolizer& text_symbolizer::operator=(text_symbolizer const& other)
{
    if (this == &other)
        return *this;
    placement_options_ = other.placement_options_; /*TODO: Copy options? */
    std::cout << "TODO: Metawriter (text_symbolizer::operator=)\n";
    return *this;
}

expression_ptr text_symbolizer::get_name() const
{
    return expression_ptr();
}

void text_symbolizer::set_name(expression_ptr name)
{
    placement_options_->properties.processor->set_old_style_expression(name);
}

expression_ptr text_symbolizer::get_orientation() const
{
    return placement_options_->properties.orientation;
}

void text_symbolizer::set_orientation(expression_ptr orientation)
{
    placement_options_->properties.orientation = orientation;
}

std::string const&  text_symbolizer::get_face_name() const
{
    return placement_options_->properties.processor->defaults.face_name;
}

void text_symbolizer::set_face_name(std::string face_name)
{
    placement_options_->properties.processor->defaults.face_name = face_name;
}

void text_symbolizer::set_fontset(font_set const& fontset)
{
    placement_options_->properties.processor->defaults.fontset = fontset;
}

font_set const& text_symbolizer::get_fontset() const
{
    return placement_options_->properties.processor->defaults.fontset;
}

unsigned  text_symbolizer::get_text_ratio() const
{
    return placement_options_->properties.text_ratio;
}

void  text_symbolizer::set_text_ratio(unsigned ratio)
{
    placement_options_->properties.text_ratio = ratio;
}

unsigned  text_symbolizer::get_wrap_width() const
{
    return placement_options_->properties.wrap_width;
}

void  text_symbolizer::set_wrap_width(unsigned width)
{
    placement_options_->properties.wrap_width = width;
}

bool  text_symbolizer::get_wrap_before() const
{
    return placement_options_->properties.processor->defaults.wrap_before;
}

void  text_symbolizer::set_wrap_before(bool wrap_before)
{
    placement_options_->properties.processor->defaults.wrap_before = wrap_before;
}

unsigned char text_symbolizer::get_wrap_char() const
{
    return placement_options_->properties.processor->defaults.wrap_char;
}

std::string text_symbolizer::get_wrap_char_string() const
{
    return std::string(1, placement_options_->properties.processor->defaults.wrap_char);
}

void  text_symbolizer::set_wrap_char(unsigned char character)
{
    placement_options_->properties.processor->defaults.wrap_char = character;
}

void  text_symbolizer::set_wrap_char_from_string(std::string const& character)
{
    placement_options_->properties.processor->defaults.wrap_char = (character)[0];
}

text_transform_e  text_symbolizer::get_text_transform() const
{
    return placement_options_->properties.processor->defaults.text_transform;
}

void  text_symbolizer::set_text_transform(text_transform_e convert)
{
    placement_options_->properties.processor->defaults.text_transform = convert;
}

unsigned  text_symbolizer::get_line_spacing() const
{
    return placement_options_->properties.processor->defaults.line_spacing;
}

void  text_symbolizer::set_line_spacing(unsigned spacing)
{
    placement_options_->properties.processor->defaults.line_spacing = spacing;
}

unsigned  text_symbolizer::get_character_spacing() const
{
    return placement_options_->properties.processor->defaults.character_spacing;
}

void  text_symbolizer::set_character_spacing(unsigned spacing)
{
    placement_options_->properties.processor->defaults.character_spacing = spacing;
}

unsigned  text_symbolizer::get_label_spacing() const
{
    return placement_options_->properties.label_spacing;
}

void  text_symbolizer::set_label_spacing(unsigned spacing)
{
    placement_options_->properties.label_spacing = spacing;
}

unsigned  text_symbolizer::get_label_position_tolerance() const
{
    return placement_options_->properties.label_position_tolerance;
}

void  text_symbolizer::set_label_position_tolerance(unsigned tolerance)
{
    placement_options_->properties.label_position_tolerance = tolerance;
}

bool  text_symbolizer::get_force_odd_labels() const
{
    return placement_options_->properties.force_odd_labels;
}

void  text_symbolizer::set_force_odd_labels(bool force)
{
    placement_options_->properties.force_odd_labels = force;
}

double text_symbolizer::get_max_char_angle_delta() const
{
    return placement_options_->properties.max_char_angle_delta;
}

void text_symbolizer::set_max_char_angle_delta(double angle)
{
    placement_options_->properties.max_char_angle_delta = angle;
}

void text_symbolizer::set_text_size(unsigned size)
{
    placement_options_->properties.processor->defaults.text_size = size;
}

unsigned  text_symbolizer::get_text_size() const
{
    return placement_options_->properties.processor->defaults.text_size;
}

void text_symbolizer::set_fill(color const& fill)
{
    placement_options_->properties.processor->defaults.fill = fill;
}

color const&  text_symbolizer::get_fill() const
{
    return placement_options_->properties.processor->defaults.fill;
}

void  text_symbolizer::set_halo_fill(color const& fill)
{
    placement_options_->properties.processor->defaults.halo_fill = fill;
}

color const&  text_symbolizer::get_halo_fill() const
{
    return placement_options_->properties.processor->defaults.halo_fill;
}

void  text_symbolizer::set_halo_radius(double radius)
{
    placement_options_->properties.processor->defaults.halo_radius = radius;
}

double text_symbolizer::get_halo_radius() const
{
    return placement_options_->properties.processor->defaults.halo_radius;
}

void  text_symbolizer::set_label_placement(label_placement_e label_p)
{
    placement_options_->properties.label_placement = label_p;
}

label_placement_e  text_symbolizer::get_label_placement() const
{
    return placement_options_->properties.label_placement;
}

void  text_symbolizer::set_displacement(double x, double y)
{
    placement_options_->properties.displacement = boost::make_tuple(x,y);
}

position const& text_symbolizer::get_displacement() const
{
    return placement_options_->properties.displacement;
}

bool text_symbolizer::get_avoid_edges() const
{
    return placement_options_->properties.avoid_edges;
}

void text_symbolizer::set_avoid_edges(bool avoid)
{
    placement_options_->properties.avoid_edges = avoid;
}

double text_symbolizer::get_minimum_distance() const
{
    return placement_options_->properties.minimum_distance;
}

void text_symbolizer::set_minimum_distance(double distance)
{
    placement_options_->properties.minimum_distance = distance;
}

double text_symbolizer::get_minimum_padding() const
{
    return placement_options_->properties.minimum_padding;
}

void text_symbolizer::set_minimum_padding(double distance)
{
    placement_options_->properties.minimum_padding = distance;
}
 
void text_symbolizer::set_allow_overlap(bool overlap)
{
    placement_options_->properties.allow_overlap = overlap;
}

bool text_symbolizer::get_allow_overlap() const
{
    return placement_options_->properties.allow_overlap;
}

void text_symbolizer::set_text_opacity(double text_opacity)
{
    placement_options_->properties.processor->defaults.text_opacity = text_opacity;
}

double text_symbolizer::get_text_opacity() const
{
    return placement_options_->properties.processor->defaults.text_opacity;
}

void text_symbolizer::set_vertical_alignment(vertical_alignment_e valign)
{
    placement_options_->properties.valign = valign;
}

vertical_alignment_e text_symbolizer::get_vertical_alignment() const
{
    return placement_options_->properties.valign;
}

void text_symbolizer::set_horizontal_alignment(horizontal_alignment_e halign)
{
    placement_options_->properties.halign = halign;
}

horizontal_alignment_e text_symbolizer::get_horizontal_alignment() const
{
    return placement_options_->properties.halign;
}

void text_symbolizer::set_justify_alignment(justify_alignment_e jalign)
{
    placement_options_->properties.jalign = jalign;
}

justify_alignment_e text_symbolizer::get_justify_alignment() const
{
    return placement_options_->properties.jalign;
}

text_placements_ptr text_symbolizer::get_placement_options() const
{
    return placement_options_;
}

void text_symbolizer::set_placement_options(text_placements_ptr placement_options)
{
    placement_options_ = placement_options;
}


}
