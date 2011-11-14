/*****************************************************************************
 * 
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
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

#ifndef MAPNIK_SQLITE_UTILS_HPP
#define MAPNIK_SQLITE_UTILS_HPP

// stl
#include <string.h>

// mapnik
#include <mapnik/datasource.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/sql_utils.hpp>


// boost
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>

// sqlite
extern "C" {
  #include <sqlite3.h>
}

#include "sqlite_resultset.hpp"
#include "sqlite_prepared.hpp"
#include "sqlite_connection.hpp"


//==============================================================================

class sqlite_utils
{
public:

    static bool needs_quoting(std::string const& name)
    {
        if (name.size() > 0)
        {
            const char first = name[0];
            if (is_quote_char(first))
            {
                return false;
            }
            if ((first >= '0' && first <= '9') ||
                (name.find("-") != std::string::npos)
               )
            {
                return true;
            }
        }
        return false;
    }

    static bool is_quote_char(const char z)
    {
        if (z == '"' || z == '\'' || z == '[' || z == '`')
        {
            return true;
        }
        return false;
    }

    static void dequote(std::string & z)
    {
        boost::algorithm::trim_if(z,boost::algorithm::is_any_of("[]'\"`"));
    }
    
    static std::string index_for_table(std::string const& table, std::string const& field)
    {
        return "\"idx_" + mapnik::sql_utils::unquote(table) + "_" + field + "\"";
    }

    static std::string index_for_db(std::string const& file)
    {
        //std::size_t idx = file.find_last_of(".");
        //if(idx != std::string::npos) {
        //    return file.substr(0,idx) + ".index" + file.substr(idx);
        //}
        //else
        //{
            return file + ".index";
        //}
    }
    
    static void get_tables(boost::shared_ptr<sqlite_connection> ds,
                           std::vector<std::string> & tables)
    {
        std::ostringstream sql;
        // todo handle finding tables from attached db's
        sql << " SELECT name FROM sqlite_master"
            << " WHERE type IN ('table','view')"
            << " AND name NOT LIKE 'sqlite_%'"
            << " AND name NOT LIKE 'idx_%'"
            << " AND name NOT LIKE '%geometry_columns%'"
            << " AND name NOT LIKE '%ref_sys%'"
            << " UNION ALL"
            << " SELECT name FROM sqlite_temp_master"
            << " WHERE type IN ('table','view')"
            << " ORDER BY 1";
        sqlite3_stmt* stmt = 0;
        const int rc = sqlite3_prepare_v2 (*(*ds), sql.str().c_str(), -1, &stmt, 0);
        std::clog << "hey\n";
        if (rc == SQLITE_OK)
        {
            std::clog << "a\n";
            boost::shared_ptr<sqlite_resultset> rs = boost::make_shared<sqlite_resultset>(stmt);
            while (rs->is_valid() && rs->step_next())
            {
            std::clog << "b\n";
                const int type_oid = rs->column_type(0);
                if (type_oid == SQLITE_TEXT)
                {
            std::clog << "c\n";
                    const char * data = rs->column_text(0);
                    if (data)
                    {

            std::clog << "d\n";
                        tables.push_back(std::string(data));
                    }
                }
            }
        }
    }    
    
    static void query_extent(boost::shared_ptr<sqlite_resultset> rs,
                             mapnik::box2d<double>& extent)
    {

        bool first = true;
        while (rs->is_valid() && rs->step_next())
        {
            int size;
            const char* data = (const char*) rs->column_blob(0, size);
            if (data)
            {
                boost::ptr_vector<mapnik::geometry_type> paths;
                mapnik::geometry_utils::from_wkb(paths, data, size, false, mapnik::wkbAuto);
                for (unsigned i=0; i<paths.size(); ++i)
                {
                    mapnik::box2d<double> const& bbox = paths[i].envelope();
    
                    if (bbox.valid())
                    {
                        if (first)
                        {
                            first = false;
                            extent = bbox;
                        }
                        else
                        {
                            extent.expand_to_include(bbox);
                        }
                    }
                }
            }
        }
    }
    
    static bool create_spatial_index(std::string const& index_db,
                                     std::string const& index_table,
                                     boost::shared_ptr<sqlite_resultset> rs)
    {
        /* TODO
          - speedups
          - return early/recover from failure
          - allow either in db or in separate
        */
        
        if (!rs->is_valid())
            return false;
        
#if SQLITE_VERSION_NUMBER >= 3005000
        int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
#else
        int flags;
#endif

        bool existed = boost::filesystem::exists(index_db);
        boost::shared_ptr<sqlite_connection> ds = boost::make_shared<sqlite_connection>(index_db,flags);

        bool one_success = false;
        try
        {
            ds->execute("PRAGMA synchronous=OFF");
            ds->execute("BEGIN IMMEDIATE TRANSACTION");
    
            // first drop the index if it already exists
            std::ostringstream spatial_index_drop_sql;
            spatial_index_drop_sql << "DROP TABLE IF EXISTS " << index_table;
            ds->execute(spatial_index_drop_sql.str());
    
            // create the spatial index
            std::ostringstream create_idx;
            create_idx << "create virtual table " 
                       << index_table
                       << " using rtree(pkid, xmin, xmax, ymin, ymax)";
    
            // insert for prepared statement
            std::ostringstream insert_idx;
            insert_idx << "insert into "
                       << index_table
                       << " values (?,?,?,?,?)";
    
            ds->execute(create_idx.str());
            
            prepared_index_statement ps(ds,insert_idx.str());

            while (rs->is_valid() && rs->step_next())
            {
                int size;
                const char* data = (const char*) rs->column_blob(0, size);
                if (data)
                {
                    boost::ptr_vector<mapnik::geometry_type> paths;
                    // TODO - contraint fails if multiple_geometries = true
                    bool multiple_geometries = false;
                    mapnik::geometry_utils::from_wkb(paths, data, size, multiple_geometries, mapnik::wkbAuto);
                    for (unsigned i=0; i<paths.size(); ++i)
                    {
                        mapnik::box2d<double> const& bbox = paths[i].envelope();
                        if (bbox.valid())
                        {
    
                            ps.bind(bbox);
        
                            const int type_oid = rs->column_type(1);
                            if (type_oid != SQLITE_INTEGER)
                            {
                                std::ostringstream error_msg;
                                error_msg << "Sqlite Plugin: invalid type for key field '"
                                          << rs->column_name(1) << "' when creating index '" << index_table
                                          << "' type was: " << type_oid << "";
                                throw mapnik::datasource_exception(error_msg.str());
                            }
        
                            const sqlite_int64 pkid = rs->column_integer64(1);
                            ps.bind(pkid);
                        }
                        else
                        {
                            std::ostringstream error_msg;
                            error_msg << "SQLite Plugin: encountered invalid bbox at '"
                                        << rs->column_name(1) << "' == " << rs->column_integer64(1);
                            throw mapnik::datasource_exception(error_msg.str());
                        }
    
                        ps.step_next();
                        one_success = true;
                    }
                }
            }
        }
        catch (mapnik::datasource_exception const& ex)
        {
            ds->execute("ROLLBACK");
            if (!existed)
            {
                try
                {
                    boost::filesystem::remove(index_db);
                }
                catch (...) {};
            }
            throw mapnik::datasource_exception(ex.what());
        }

        if (one_success)
        {
            ds->execute("COMMIT");
            return true;
        }
        else if (!existed)
        {
            ds->execute("ROLLBACK");
            try
            {
                boost::filesystem::remove(index_db);
            }
            catch (...) {};
        }
        return false;
    }

    static bool detect_extent(boost::shared_ptr<sqlite_connection> ds,
                              bool has_spatial_index,
                              mapnik::box2d<double> & extent,
                              std::string const& index_table,
                              std::string const& metadata,
                              std::string const& geometry_field,
                              std::string const& geometry_table,
                              std::string const& key_field,
                              std::string const& table
                              )
    {
        if (has_spatial_index)
        {
            std::ostringstream s;
            s << "SELECT MIN(xmin), MIN(ymin), MAX(xmax), MAX(ymax) FROM " 
            << index_table;
    
            boost::shared_ptr<sqlite_resultset> rs(ds->execute_query(s.str()));
            if (rs->is_valid() && rs->step_next())
            {
                if (! rs->column_isnull(0))
                {
                    try 
                    {
                        double xmin = boost::lexical_cast<double>(rs->column_double(0));
                        double ymin = boost::lexical_cast<double>(rs->column_double(1));
                        double xmax = boost::lexical_cast<double>(rs->column_double(2));
                        double ymax = boost::lexical_cast<double>(rs->column_double(3));
                        extent.init(xmin, ymin, xmax, ymax);
                        return true;
                    }
                    catch (boost::bad_lexical_cast& ex)
                    {
                        std::ostringstream ss;
                        ss << "SQLite Plugin: warning: could not determine extent from query: "
                           << "'" << s.str() << "' \n problem was: " << ex.what() << std::endl;
                        std::clog << ss.str();
                    }
                }
            }
        }
        else if (! metadata.empty())
        {
            std::ostringstream s;
            s << "SELECT xmin, ymin, xmax, ymax FROM " << metadata;
            s << " WHERE LOWER(f_table_name) = LOWER('" << geometry_table << "')";
            boost::shared_ptr<sqlite_resultset> rs(ds->execute_query(s.str()));
            if (rs->is_valid() && rs->step_next())
            {
                double xmin = rs->column_double (0);
                double ymin = rs->column_double (1);
                double xmax = rs->column_double (2);
                double ymax = rs->column_double (3);
                extent.init (xmin, ymin, xmax, ymax);
                return true;
            }
        }
        else if (! key_field.empty())
        {
            std::ostringstream s;
            s << "SELECT " << geometry_field << "," << key_field
              << " FROM (" << table << ")";
            boost::shared_ptr<sqlite_resultset> rs(ds->execute_query(s.str()));
            sqlite_utils::query_extent(rs,extent);
            return true;
        }
        return false;
    }

    static bool has_rtree(std::string const& index_table,boost::shared_ptr<sqlite_connection> ds)
    {
        try
        {
            std::ostringstream s;
            s << "SELECT pkid,xmin,xmax,ymin,ymax FROM " << index_table << " LIMIT 1";
            boost::shared_ptr<sqlite_resultset> rs = ds->execute_query(s.str());
            if (rs->is_valid() && rs->step_next())
            {
                return true;
            }
        }
        catch (std::exception const& ex)
        {
            //std::clog << "no: " << ex.what() << "\n";
            return false;
        }
        return false;
    }

    static bool detect_types_from_subquery(std::string const& query,
                                    std::string & geometry_field,
                                    mapnik::layer_descriptor & desc,
                                    boost::shared_ptr<sqlite_connection> ds)
    {
        bool found = false;
        boost::shared_ptr<sqlite_resultset> rs(ds->execute_query(query));
        if (rs->is_valid() && rs->step_next())
        {
            for (int i = 0; i < rs->column_count(); ++i)
            {
                found = true;
                const int type_oid = rs->column_type(i);
                const char* fld_name = rs->column_name(i);
                switch (type_oid)
                {
                case SQLITE_INTEGER:
                    desc.add_descriptor(mapnik::attribute_descriptor(fld_name, mapnik::Integer));
                    break;
                     
                case SQLITE_FLOAT:
                    desc.add_descriptor(mapnik::attribute_descriptor(fld_name, mapnik::Double));
                    break;
                     
                case SQLITE_TEXT:
                    desc.add_descriptor(mapnik::attribute_descriptor(fld_name, mapnik::String));
                    break;
                     
                case SQLITE_NULL:
                    // sqlite reports based on value, not actual column type unless
                    // PRAGMA table_info is used so here we assume the column is a string
                    // which is a lesser evil than altogether dropping the column
                    desc.add_descriptor(mapnik::attribute_descriptor(fld_name, mapnik::String));

                case SQLITE_BLOB:
                    if (geometry_field.empty()
                        && (boost::algorithm::icontains(fld_name, "geom") ||
                            boost::algorithm::icontains(fld_name, "point") ||
                            boost::algorithm::icontains(fld_name, "linestring") ||
                            boost::algorithm::icontains(fld_name, "polygon")))
                    {
                        geometry_field = std::string(fld_name);
                    }
                    break;
                     
                default:
#ifdef MAPNIK_DEBUG
                    std::clog << "Sqlite Plugin: unknown type_oid=" << type_oid << std::endl;
#endif
                    break;
                }
            }
        }
        
        return found;
    }

    static bool table_info(std::string & key_field,
                    bool detected_types,
                    std::string & field,
                    std::string & table,
                    mapnik::layer_descriptor & desc,
                    boost::shared_ptr<sqlite_connection> ds)
    {

        // http://www.sqlite.org/pragma.html#pragma_table_info
        // use pragma table_info to detect primary key
        // and to detect types if no subquery is used or 
        // if the subquery-based type detection failed
        std::ostringstream s;
        s << "PRAGMA table_info(" << table << ")";
        boost::shared_ptr<sqlite_resultset> rs(ds->execute_query(s.str()));
        bool found_table = false;
        bool found_pk = false;
        while (rs->is_valid() && rs->step_next())
        {
            found_table = true;
            const char* fld_name = rs->column_text(1);
            std::string fld_type(rs->column_text(2));
            int fld_pk = rs->column_integer(5);
            boost::algorithm::to_lower(fld_type);
    
            // TODO - how to handle primary keys on multiple columns ?
            if (key_field.empty() && ! found_pk && fld_pk != 0)
            {
                key_field = fld_name;
                found_pk = true;
            }
            if (! detected_types)
            {
                // see 2.1 "Column Affinity" at http://www.sqlite.org/datatype3.html
                // TODO - refactor this somehow ?
                if (field.empty()
                    && ((boost::algorithm::contains(fld_type, "geom") ||
                         boost::algorithm::contains(fld_type, "point") ||
                         boost::algorithm::contains(fld_type, "linestring") ||
                         boost::algorithm::contains(fld_type, "polygon"))
                    ||
                        (boost::algorithm::icontains(fld_name, "geom") ||
                         boost::algorithm::icontains(fld_name, "point") ||
                         boost::algorithm::icontains(fld_name, "linestring") ||
                         boost::algorithm::icontains(fld_name, "polygon")))
                   )
                {
                    field = std::string(fld_name);
                }
                else if (boost::algorithm::contains(fld_type, "int"))
                {
                    desc.add_descriptor(mapnik::attribute_descriptor(fld_name, mapnik::Integer));
                }
                else if (boost::algorithm::contains(fld_type, "text") ||
                         boost::algorithm::contains(fld_type, "char") ||
                         boost::algorithm::contains(fld_type, "clob"))
                {
                    desc.add_descriptor(mapnik::attribute_descriptor(fld_name, mapnik::String));
                }
                else if (boost::algorithm::contains(fld_type, "real") ||
                         boost::algorithm::contains(fld_type, "float") ||
                         boost::algorithm::contains(fld_type, "double"))
                {
                    desc.add_descriptor(mapnik::attribute_descriptor(fld_name, mapnik::Double));
                }
                else if (boost::algorithm::contains(fld_type, "blob"))
                {
                    if (! field.empty())
                    {
                        desc.add_descriptor(mapnik::attribute_descriptor(fld_name, mapnik::String));
                    }
                }
        #ifdef MAPNIK_DEBUG
                else
                {
                    // "Column Affinity" says default to "Numeric" but for now we pass..
                    //desc_.add_descriptor(attribute_descriptor(fld_name,mapnik::Double));
        
                    // TODO - this should not fail when we specify geometry_field in XML file
        
                    std::clog << "Sqlite Plugin: column '"
                              << std::string(fld_name)
                              << "' unhandled due to unknown type: "
                              << fld_type << std::endl;
                }
        #endif
            }
        }
        
        return found_table;
    }
};

#endif // MAPNIK_SQLITE_UTILS_HPP