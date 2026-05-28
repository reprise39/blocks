from setuptools import find_packages, setup


with open('requirements.txt') as requirements_file:
    install_requirements = requirements_file.read().splitlines()


setup(
    name='mygame_ai',
    version='0.1.0',
    description='C++ blocks-duo AI bridge',
    packages=find_packages(),
    install_requires=install_requirements,
    entry_points={
        'console_scripts': [
            'mygame_ai=mygame_ai.main:main',
            'mygame_ai_2=mygame_ai.main:main_alt',
            'mygame_chokudai=mygame_ai.main:main_chokudai',
        ]
    },
)