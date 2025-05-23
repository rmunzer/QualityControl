import logging
from typing import Dict

from qcrepocleaner.Ccdb import Ccdb

logger = logging  # default logger


def process(ccdb: Ccdb, object_path: str, delay: int,  from_timestamp: int, to_timestamp: int, extra_params: Dict[str, str]):
    """
    Process this deletion rule on the object. We use the CCDB passed by argument.

    This policy does nothing and allows to preserve some directories.

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    :param from_timestamp: only objects created after this timestamp are considered.
    :param to_timestamp: only objects created before this timestamp are considered.
    :param extra_params: a dictionary containing extra parameters (unused in this rule)
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    """
    
    logger.debug(f"Plugin 'skip' processing {object_path}")

    versions = ccdb.getVersionsList(object_path)

    return {"deleted": 0, "preserved": len(versions), "updated": 0}