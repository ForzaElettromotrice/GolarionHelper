import logging
import colorlog

handler = colorlog.StreamHandler()
handler.setFormatter(colorlog.ColoredFormatter(
    # "%(log_color)s[%(levelname)s]%(reset)s - %(asctime)s - %(message)s",
    "%(log_color)s[%(levelname)s]%(reset)s - %(asctime)s.%(msecs)03d - %(message)s",
    # datefmt="%Y-%m-%d %H:%M:%S",
    datefmt="%H:%M:%S",
    log_colors={
        'DEBUG': 'cyan',
        'INFO': 'green',
        'WARNING': 'yellow',
        'ERROR': 'red',
        'CRITICAL': 'bold_red',
    }
))

logger = logging.getLogger()
logger.addHandler(handler)
logger.setLevel(logging.DEBUG)

def log_info(message):
    logger.info(message)

def log_error(message):
    logger.error(message)

def log_warning(message):
    logger.warning(message)

def log_debug(message):
    logger.debug(message)
