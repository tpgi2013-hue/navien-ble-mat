import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate

from . import navien_mat_ns

NavienClimate = navien_mat_ns.class_("NavienClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.climate_schema(NavienClimate).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    cg.add_global(
        cg.RawStatement('#include "esphome/components/navien_mat/navien_climate.h"')
    )
    var = await climate.new_climate(config)
    await cg.register_component(var, config)
